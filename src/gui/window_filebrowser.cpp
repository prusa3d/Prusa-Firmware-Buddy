/**
 * @file window_filebrowser.cpp
 */

#include "window_filebrowser.hpp"
#include "log.h"
#include "sound.hpp"
#include <transfers/transfer.hpp>
#include <algorithm>

LOG_COMPONENT_REF(GUI);

#ifndef MAXPATHNAMELENGTH
    #define MAXPATHNAMELENGTH F_MAXPATHNAMELENGTH
#endif

/// To save first/top visible item in the file browser
/// This is something else than the selected file for print
/// This is used to restore the content of the browser into previous state including the layout
constexpr unsigned int SFN_len = 13;
static char firstVisibleSFN[SFN_len];
char WindowFileBrowser::root[FILE_PATH_BUFFER_LEN] = "/usb";

static constexpr char dirUp[] = "..";
static constexpr char slash = '/';

WindowFileBrowser::WindowFileBrowser(window_t *parent, Rect16 rect, const char *media_SFN_path)
    : AddSuperWindow<window_file_list_t>(parent, rect) {

    // set root of the file list
    window_file_list_t::SetRoot(root);
    // initialize the directory
    // here the strncpy is meant to be - need the rest of the buffer zeroed
    strncpy(sfn_path, media_SFN_path, sizeof(sfn_path));
    // ensure null character at the end no matter what
    sfn_path[sizeof(sfn_path) - 1] = '\0';
    // cut by the filename to retain only the directory path
    char *c = strrchr(sfn_path, '/');
    *c = 0; // even if we didn't find the '/', c will point to valid memory
    // check if we are at least in the root directory, if not move to root directory
    if (strstr(sfn_path, root) != sfn_path) {
        strlcpy(sfn_path, root, FILE_PATH_BUFFER_LEN);
    }
    // Moreover - the next characters after c contain the filename, which I want to start my cursor at!
    Load(GuiFileSort::Get(), c + 1, firstVisibleSFN);

    // scroll offset/current item is automatically set by IWindowMenu
}

void WindowFileBrowser::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    switch (event) {

    case GUI_event_t::CLICK:
        handle_click();
        return;

    case GUI_event_t::TOUCH:
        if (move_focus_touch_click(param)) {
            handle_click();
        }
        return;

    default:
        break;
    }

    SuperWindowEvent(sender, event, param);
}

void WindowFileBrowser::CopyRootTo(char *path) {
    strlcpy(path, root, FILE_PATH_BUFFER_LEN);
}

void WindowFileBrowser::SetRoot(const char *path) {
    strlcpy(root, path, FILE_PATH_BUFFER_LEN);
}

void WindowFileBrowser::SaveTopSFN() {
    strlcpy(firstVisibleSFN, TopItemSFN(), SFN_len);
}

int WindowFileBrowser::WriteNameToPrint(char *buff, size_t sz) {
    return snprintf(buff, sz, "%s/%s", sfn_path, CurrentSFN());
}

void WindowFileBrowser::handle_click() {
    const auto focused_slot_opt = this->focused_slot();

    // No item focused -> do nothign
    if (!focused_slot_opt) {
        return;
    }

    const auto focused_slot = *focused_slot_opt;

    // Return button -> signal to parent
    if (is_return_slot(focused_slot)) {
        log_debug(GUI, "Clicked on return slot");
        go_up();
        return;
    }

    bool is_item_file;
    const char *currentSFN = CurrentSFN(&is_item_file);
    log_debug(GUI, "Clicked on item: %s", currentSFN);

    size_t sfn_path_len = strlen(sfn_path);
    if ((sfn_path_len + strlen(currentSFN) + 1) >= MAXPATHNAMELENGTH) {
        log_error(GUI, "Path too long");
        return;
    }

    // Item is directory -> open the directory, do not report to the parent
    if (!is_item_file) {
        // We've selected a subdirectory (filename != "..") -> open the dir
        if (strcmp(currentSFN, dirUp)) {
            // append the dir name at the end of sfnPath
            if (sfn_path[sfn_path_len - 1] != slash) {
                sfn_path[sfn_path_len++] = slash;
            }

            strlcpy(sfn_path + sfn_path_len, currentSFN, FILE_PATH_BUFFER_LEN - sfn_path_len);

            Load(GuiFileSort::Get(), nullptr, nullptr);
        }

        // We've selected ".." and are not in the top level dir (sfn_path contains '/') -> remove the directory
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
    char *last_slash_char = strrchr(sfn_path, '/');

    // We're in the root -> signal to parent
    if (!last_slash_char || IsPathRoot(sfn_path)) {
        event_conversion_union un {
            .action = event_conversion_union::Action::go_home,
        };
        GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
        return;
    }

    char previous_dir_sfn[FILE_PATH_BUFFER_LEN];
    strlcpy(previous_dir_sfn, last_slash_char + 1, sizeof(previous_dir_sfn));

    // We are in the top level directory (sfn_path == "/something", last_slash_char is the first char in the path)
    // -> make the string into "/"
    if (last_slash_char == sfn_path) {
        sfn_path[1] = '\0';
    }

    // We're not in the top level directory (sfn_path = "/abc/def" -> cut the string after the last '/')
    else {
        *last_slash_char = '\0';
    }

    Load(GuiFileSort::Get(), previous_dir_sfn, previous_dir_sfn);

    // @@TODO we want to print the LFN of the dir name, which is very hard to do right now
    // However, the text is not visible on the screen yet...
    // header.SetText(strrchr(sfn_path, '/'));
}
