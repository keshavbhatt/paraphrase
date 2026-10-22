#pragma once

#include "backends.h"
#include "data_control_client.h"
#include "window_tracker.h"

#include <memory>

namespace paraphrase::app {

/// The highlighted text, else the clipboard, on Wayland without focus (data-control), with
/// the window it came from.
class DataControlCapture final : public CaptureBackend
{
public:
    DataControlCapture(std::shared_ptr<DataControlClient> client, std::shared_ptr<WindowTracker> tracker)
        : m_client(std::move(client))
        , m_tracker(std::move(tracker))
    {}

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return m_client != nullptr; }
    [[nodiscard]] std::optional<CapturedText> capture() override;

private:
    std::shared_ptr<DataControlClient> m_client;
    std::shared_ptr<WindowTracker> m_tracker;
};

/// X11: the PRIMARY selection (readable without focus), else the clipboard, with the window
/// it came from.
class X11SelectionCapture final : public CaptureBackend
{
public:
    explicit X11SelectionCapture(std::shared_ptr<WindowTracker> tracker)
        : m_tracker(std::move(tracker))
    {}

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::optional<CapturedText> capture() override;

private:
    std::shared_ptr<WindowTracker> m_tracker;
};

/// The window and app part of a capture, from a tracker that may be unavailable.
void markSource(CapturedText& text, const WindowTracker* tracker);

} // namespace paraphrase::app
