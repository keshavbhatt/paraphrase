#pragma once

#include "clipboard_access.h"

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <memory>
#include <optional>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct ext_data_control_manager_v1;
struct ext_data_control_device_v1;
struct ext_data_control_offer_v1;
struct ext_data_control_source_v1;
class QSocketNotifier;

namespace paraphrase::app {

/// The clipboard and the selection on Wayland without having focus: the ext-data-control-v1
/// protocol (KDE Plasma 6, wlroots compositors), on a connection of its own so Qt's is never
/// disturbed. It follows the current offers as the compositor announces them, reads them on
/// request and can own the clipboard to paste a result. Nothing it reads is logged.
class DataControlClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DataControlClient)

public:
    enum class Which : std::uint8_t
    {
        Clipboard,
        Selection, ///< the primary selection: what is highlighted now
    };
    static constexpr int kReadTimeoutMs = 500;

    /// A client for this session, or nullptr when the compositor lacks the protocol.
    [[nodiscard]] static std::unique_ptr<DataControlClient> connectToCompositor();
    ~DataControlClient() override;

    [[nodiscard]] std::optional<QString> readText(Which which);
    [[nodiscard]] QStringList mimeTypes(Which which) const;
    [[nodiscard]] QByteArray read(Which which, const QString& mime);
    /// Owns the clipboard with these formats until another app copies something.
    void setClipboard(const ClipboardContent& content);

    // Protocol callbacks (C listeners forward here).
    void onGlobal(wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    void onDataOffer(ext_data_control_offer_v1* offer);
    void onOfferMime(ext_data_control_offer_v1* offer, const char* mime);
    void onSelection(ext_data_control_offer_v1* offer, Which which);
    void onSourceSend(ext_data_control_source_v1* source, const char* mime, int fd);
    void onSourceCancelled(ext_data_control_source_v1* source);

private:
    explicit DataControlClient(wl_display* display);
    [[nodiscard]] bool setUp();
    void dispatch();
    void sync();
    [[nodiscard]] ext_data_control_offer_v1* current(Which which) const;
    void dropOffer(ext_data_control_offer_v1* offer);

    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    wl_seat* m_seat = nullptr;
    ext_data_control_manager_v1* m_manager = nullptr;
    ext_data_control_device_v1* m_device = nullptr;
    QSocketNotifier* m_notifier = nullptr;
    QHash<ext_data_control_offer_v1*, QStringList> m_offers;
    ext_data_control_offer_v1* m_clipboard = nullptr;
    ext_data_control_offer_v1* m_selection = nullptr;
    ext_data_control_source_v1* m_source = nullptr;
    ClipboardContent m_owned; ///< what we serve while we own the clipboard
};

/// ClipboardAccess through data-control: works while a Paraphrase window has no focus.
class DataControlClipboard final : public ClipboardAccess
{
public:
    static constexpr qsizetype kMaxSaved = qsizetype{16} * 1024 * 1024; ///< bigger clipboards are not kept

    explicit DataControlClipboard(std::shared_ptr<DataControlClient> client)
        : m_client(std::move(client))
    {}
    [[nodiscard]] ClipboardContent save() override;
    void setText(const QString& text) override;
    void restore(const ClipboardContent& content) override;

private:
    std::shared_ptr<DataControlClient> m_client;
};

} // namespace paraphrase::app
