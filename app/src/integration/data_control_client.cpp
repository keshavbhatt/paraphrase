#include "data_control_client.h"

#include "ext-data-control-v1-client-protocol.h"

#include <QDeadlineTimer>
#include <QSocketNotifier>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <wayland-client.h>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr uint32_t kSeatVersion = 1;
constexpr uint32_t kManagerVersion = 1;
constexpr qsizetype kChunk = qsizetype{64} * 1024;

/// Text formats in the order they are asked for.
const QStringList& textMimes()
{
    static const QStringList kMimes = {
        u"text/plain;charset=utf-8"_s, u"text/plain"_s, u"UTF8_STRING"_s, u"TEXT"_s, u"STRING"_s,
    };
    return kMimes;
}

DataControlClient* self(void* data)
{
    return static_cast<DataControlClient*>(data);
}

constexpr wl_registry_listener kRegistryListener = {
    .global = [](void* data, wl_registry* registry, uint32_t name, const char* interface,
                 uint32_t version) { self(data)->onGlobal(registry, name, interface, version); },
    .global_remove = [](void* /*data*/, wl_registry* /*registry*/, uint32_t /*name*/) {},
};

constexpr ext_data_control_offer_v1_listener kOfferListener = {
    .offer = [](void* data, ext_data_control_offer_v1* offer,
                const char* mime) { self(data)->onOfferMime(offer, mime); },
};

constexpr ext_data_control_device_v1_listener kDeviceListener = {
    .data_offer = [](void* data, ext_data_control_device_v1* /*device*/,
                     ext_data_control_offer_v1* offer) { self(data)->onDataOffer(offer); },
    .selection =
        [](void* data, ext_data_control_device_v1* /*device*/, ext_data_control_offer_v1* offer) {
            self(data)->onSelection(offer, DataControlClient::Which::Clipboard);
        },
    .finished = [](void* /*data*/, ext_data_control_device_v1* /*device*/) {},
    .primary_selection =
        [](void* data, ext_data_control_device_v1* /*device*/, ext_data_control_offer_v1* offer) {
            self(data)->onSelection(offer, DataControlClient::Which::Selection);
        },
};

constexpr ext_data_control_source_v1_listener kSourceListener = {
    .send = [](void* data, ext_data_control_source_v1* source, const char* mime,
               int32_t fd) { self(data)->onSourceSend(source, mime, fd); },
    .cancelled = [](void* data,
                    ext_data_control_source_v1* source) { self(data)->onSourceCancelled(source); },
};

/// Reads a pipe until the other end closes it or the time is up.
QByteArray readAll(int fd, int timeoutMs)
{
    QByteArray out;
    const QDeadlineTimer deadline(timeoutMs);
    pollfd p{.fd = fd, .events = POLLIN, .revents = 0};
    while (!deadline.hasExpired() && ::poll(&p, 1, static_cast<int>(deadline.remainingTime())) > 0) {
        char buffer[kChunk];
        const ssize_t n = ::read(fd, buffer, sizeof buffer);
        if (n <= 0) {
            break; // end of data, or an error
        }
        out.append(buffer, n);
    }
    return out;
}

void writeAll(int fd, const QByteArray& data)
{
    qsizetype done = 0;
    while (done < data.size()) {
        const ssize_t n = ::write(fd, data.constData() + done, static_cast<size_t>(data.size() - done));
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            break; // the reader went away
        }
        done += n;
    }
}

} // namespace

DataControlClient::DataControlClient(wl_display* display)
    : m_display(display)
{}

std::unique_ptr<DataControlClient> DataControlClient::connectToCompositor()
{
    wl_display* display = wl_display_connect(nullptr);
    if (display == nullptr) {
        return nullptr;
    }
    std::unique_ptr<DataControlClient> client(new DataControlClient(display));
    return client->setUp() ? std::move(client) : nullptr;
}

bool DataControlClient::setUp()
{
    // An app reading the clipboard may close its end early; writing to it must not end ours.
    struct sigaction current = {};
    if (::sigaction(SIGPIPE, nullptr, &current) == 0 && current.sa_handler == SIG_DFL) {
        ::signal(SIGPIPE, SIG_IGN);
    }
    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &kRegistryListener, this);
    wl_display_roundtrip(m_display);
    if (m_manager == nullptr || m_seat == nullptr) {
        return false;
    }
    m_device = ext_data_control_manager_v1_get_data_device(m_manager, m_seat);
    ext_data_control_device_v1_add_listener(m_device, &kDeviceListener, this);
    wl_display_roundtrip(m_display); // the current offers arrive now
    m_notifier = new QSocketNotifier(wl_display_get_fd(m_display), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &DataControlClient::dispatch);
    return true;
}

DataControlClient::~DataControlClient()
{
    for (auto it = m_offers.begin(); it != m_offers.end(); ++it) {
        ext_data_control_offer_v1_destroy(it.key());
    }
    if (m_source != nullptr) {
        ext_data_control_source_v1_destroy(m_source);
    }
    if (m_device != nullptr) {
        ext_data_control_device_v1_destroy(m_device);
    }
    if (m_manager != nullptr) {
        ext_data_control_manager_v1_destroy(m_manager);
    }
    if (m_seat != nullptr) {
        wl_seat_destroy(m_seat);
    }
    if (m_registry != nullptr) {
        wl_registry_destroy(m_registry);
    }
    wl_display_disconnect(m_display);
}

void DataControlClient::onGlobal(wl_registry* registry, uint32_t name, const char* interface,
                                 uint32_t /*version*/)
{
    if (std::strcmp(interface, ext_data_control_manager_v1_interface.name) == 0 && m_manager == nullptr) {
        m_manager = static_cast<ext_data_control_manager_v1*>(
            wl_registry_bind(registry, name, &ext_data_control_manager_v1_interface, kManagerVersion));
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0 && m_seat == nullptr) {
        m_seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, kSeatVersion));
    }
}

void DataControlClient::onDataOffer(ext_data_control_offer_v1* offer)
{
    m_offers.insert(offer, {});
    ext_data_control_offer_v1_add_listener(offer, &kOfferListener, this);
}

void DataControlClient::onOfferMime(ext_data_control_offer_v1* offer, const char* mime)
{
    m_offers[offer].append(QString::fromUtf8(mime));
}

void DataControlClient::onSelection(ext_data_control_offer_v1* offer, Which which)
{
    ext_data_control_offer_v1*& slot = which == Which::Clipboard ? m_clipboard : m_selection;
    ext_data_control_offer_v1* old = slot;
    slot = offer;
    if (old != nullptr && old != m_clipboard && old != m_selection) {
        dropOffer(old);
    }
}

void DataControlClient::dropOffer(ext_data_control_offer_v1* offer)
{
    m_offers.remove(offer);
    ext_data_control_offer_v1_destroy(offer);
}

void DataControlClient::onSourceSend(ext_data_control_source_v1* /*source*/, const char* mime, int fd)
{
    const QString wanted = QString::fromUtf8(mime);
    for (const auto& [type, data] : std::as_const(m_owned)) {
        if (type == wanted) {
            writeAll(fd, data);
            break;
        }
    }
    ::close(fd);
}

void DataControlClient::onSourceCancelled(ext_data_control_source_v1* source)
{
    // Another app copied something: we no longer own the clipboard.
    if (source == m_source) {
        m_source = nullptr;
        m_owned.clear();
    }
    ext_data_control_source_v1_destroy(source);
}

void DataControlClient::dispatch()
{
    while (wl_display_prepare_read(m_display) != 0) {
        wl_display_dispatch_pending(m_display);
    }
    wl_display_flush(m_display);
    if (wl_display_read_events(m_display) < 0) {
        m_notifier->setEnabled(false); // the compositor went away
        return;
    }
    wl_display_dispatch_pending(m_display);
}

void DataControlClient::sync()
{
    wl_display_roundtrip(m_display);
}

ext_data_control_offer_v1* DataControlClient::current(Which which) const
{
    return which == Which::Clipboard ? m_clipboard : m_selection;
}

QStringList DataControlClient::mimeTypes(Which which) const
{
    if (which == Which::Clipboard && m_source != nullptr) {
        QStringList types;
        for (const auto& entry : m_owned) {
            types.append(entry.first);
        }
        return types;
    }
    return m_offers.value(current(which));
}

QByteArray DataControlClient::read(Which which, const QString& mime)
{
    if (which == Which::Clipboard && m_source != nullptr) {
        // Our own offer: reading it through the compositor would wait for ourselves.
        for (const auto& [type, data] : std::as_const(m_owned)) {
            if (type == mime) {
                return data;
            }
        }
        return {};
    }
    ext_data_control_offer_v1* offer = current(which);
    int fds[2] = {-1, -1};
    if (offer == nullptr || ::pipe2(fds, O_CLOEXEC) != 0) {
        return {};
    }
    ext_data_control_offer_v1_receive(offer, mime.toUtf8().constData(), fds[1]);
    wl_display_flush(m_display);
    ::close(fds[1]);
    const QByteArray data = readAll(fds[0], kReadTimeoutMs);
    ::close(fds[0]);
    return data;
}

std::optional<QString> DataControlClient::readText(Which which)
{
    sync();
    const QStringList offered = mimeTypes(which);
    for (const QString& mime : textMimes()) {
        if (offered.contains(mime)) {
            return QString::fromUtf8(read(which, mime));
        }
    }
    return std::nullopt;
}

void DataControlClient::setClipboard(const ClipboardContent& content)
{
    if (m_source != nullptr) {
        ext_data_control_source_v1_destroy(m_source);
        m_source = nullptr;
    }
    m_owned = content;
    if (content.isEmpty()) {
        ext_data_control_device_v1_set_selection(m_device, nullptr);
        wl_display_flush(m_display);
        return;
    }
    m_source = ext_data_control_manager_v1_create_data_source(m_manager);
    ext_data_control_source_v1_add_listener(m_source, &kSourceListener, this);
    for (const auto& entry : content) {
        ext_data_control_source_v1_offer(m_source, entry.first.toUtf8().constData());
    }
    ext_data_control_device_v1_set_selection(m_device, m_source);
    sync();
}

ClipboardContent DataControlClipboard::save()
{
    ClipboardContent saved;
    qsizetype size = 0;
    for (const QString& mime : m_client->mimeTypes(DataControlClient::Which::Clipboard)) {
        QByteArray data = m_client->read(DataControlClient::Which::Clipboard, mime);
        size += data.size();
        if (size > kMaxSaved) {
            break; // keep what fits; a huge image is not worth holding
        }
        saved.append({mime, std::move(data)});
    }
    return saved;
}

void DataControlClipboard::setText(const QString& text)
{
    ClipboardContent content;
    const QByteArray utf8 = text.toUtf8();
    for (const QString& mime : textMimes()) {
        content.append({mime, utf8});
    }
    m_client->setClipboard(content);
}

void DataControlClipboard::restore(const ClipboardContent& content)
{
    m_client->setClipboard(content);
}

} // namespace paraphrase::app
