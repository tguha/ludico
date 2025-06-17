#include "ui/cursor_attachment.hpp"
#include "ui/ui_root.hpp"
#include "global.hpp"

std::unique_ptr<CursorAttachment> CursorAttachment::remove() {
    ASSERT(global.ui->cursor_attachment.get() == this);
    return std::move(global.ui->cursor_attachment);
}
