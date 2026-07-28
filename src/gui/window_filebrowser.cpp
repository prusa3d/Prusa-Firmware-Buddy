/**
 * @file window_filebrowser.cpp
 */

#include "window_filebrowser.hpp"
#include <logging/log.hpp>
#include "sound.hpp"
#include <window_msgbox.hpp>
#include "i18n.h"
#include <transfers/transfer.hpp>
#include <buddy/filename_defs.hpp>
#include <algorithm>

LOG_COMPONENT_REF(GUI);

/// To save first/top visible item in the file browser
/// This is something else than the selected file for print
/// This is used to restore the content of the browser into previous state including the layout
static char first_visible_canonical[filename_defs::filename_buffer_size];

static constexpr char dirUp[] = "..";
static constexpr char slash = '/';

WindowFileBrowser::WindowFileBrowser(window_t *parent, Rect16 rect, const char *media_path)
    : window_file_list_t(parent, rect) {

    // initialize the directory
    // here the strncpy is meant to be - need the rest of the buffer zeroed
    strncpy(canonical_path, media_path, sizeof(canonical_path));
    // ensure null character at the end no matter what
    canonical_path[sizeof(canonical_path) - 1] = '\0';
    char *canonical_at_cursor;
    if (char *c = strrchr(canonical_path, '/')) {
        // cut by the filename to retain only the directory path
        *c = 0;
        // the next characters after c contain the filename
        canonical_at_cursor = c + 1;
    } else {
        canonical_at_cursor = nullptr;
    }
    // check if we are at least in the root directory, if not move to root directory
    if (strstr(canonical_path, root) != canonical_path) {
        strlcpy(canonical_path, root, filename_defs::path_buffer_size);
        // invalidate cursor if we had to rewrite the path
        canonical_at_cursor = nullptr;
    }
    Load(GuiFileSort::Get(), canonical_at_cursor, first_visible_canonical);

    // scroll offset/current item is automatically set by IWindowMenu
}

void WindowFileBrowser::windowEvent(window_t *sender, GUI_event_t event, void *param) {

    switch (event) {

    case GUI_event_t::CLICK:
        handle_click();
        return;

    case GUI_event_t::TOUCH_CLICK:
        if (move_focus_touch_click(param)) {
            handle_click();
        }
        return;

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        sound::play(SoundType::button_echo);
        go_up();
        return;

    default:
        break;
    }

    window_file_list_t::windowEvent(sender, event, param);
}

void WindowFileBrowser::save_top_canonical() {
    strlcpy(first_visible_canonical, top_item_canonical(), sizeof(first_visible_canonical));
}

void WindowFileBrowser::clear_first_visible_canonical() {
    *first_visible_canonical = 0;
}

int WindowFileBrowser::WriteNameToPrint(char *buff, size_t sz) {
    return snprintf(buff, sz, "%s/%s", canonical_path, current_canonical());
}

void WindowFileBrowser::handle_click() {
    const auto focused_slot_opt = this->focused_slot();

    // No item focused -> do nothign
    if (!focused_slot_opt) {
        return;
    }

    bool is_item_file;
    const char *item_canonical = current_canonical(&is_item_file);
    log_debug(GUI, "Clicked on item: %s", item_canonical);

    size_t path_len = strlen(canonical_path);
    if ((path_len + strlen(item_canonical) + 1) >= filename_defs::path_buffer_size) {
        log_error(GUI, "Path too long");
        MsgBoxError(_("The path is too long."), Responses_Ok);
        return;
    }

    // Item is directory -> open the directory, do not report to the parent
    if (!is_item_file) {
        // We've selected a subdirectory (filename != "..") -> open the dir
        if (strcmp(item_canonical, dirUp)) {
            // append the dir name at the end of canonical_path
            if (canonical_path[path_len - 1] != slash) {
                canonical_path[path_len++] = slash;
            }

            strlcpy(canonical_path + path_len, item_canonical, filename_defs::path_buffer_size - path_len);

            Load(GuiFileSort::Get(), nullptr, nullptr);
        }

        // We've selected ".." and are not in the top level dir (canonical_path contains '/') -> remove the directory
        else {
            go_up();
        }
    }

    // Item is a file -> report file click to the parent
    else if (auto parent = GetParent()) {
        event_conversion_union un {
            .action = event_conversion_union::Action::file_selected,
        };
        parent->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
    }
}

void WindowFileBrowser::go_up() {
    char *last_slash_char = strrchr(canonical_path, '/');

    // We're in the root -> signal to parent
    if (!last_slash_char || IsPathRoot(canonical_path)) {
        event_conversion_union un {
            .action = event_conversion_union::Action::go_home,
        };
        GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
        return;
    }

    char previous_dir_canonical[filename_defs::filename_buffer_size];
    strlcpy(previous_dir_canonical, last_slash_char + 1, sizeof(previous_dir_canonical));

    // We are in the top level directory (canonical_path == "/something", last_slash_char is the first char in the path)
    // -> make the string into "/"
    if (last_slash_char == canonical_path) {
        canonical_path[1] = '\0';
    }

    // We're not in the top level directory (canonical_path = "/abc/def" -> cut the string after the last '/')
    else {
        *last_slash_char = '\0';
    }

    Load(GuiFileSort::Get(), previous_dir_canonical, nullptr);

    // @@TODO we want to print the LFN of the dir name, which is very hard to do right now
    // However, the text is not visible on the screen yet...
    // header.SetText(strrchr(canonical_path, '/'));
}
