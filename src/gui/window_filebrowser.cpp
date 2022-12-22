/**
 * @file window_filebrowser.cpp
 */

#include "window_filebrowser.hpp"
#include "log.h"
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

WindowFileBrowser::WindowFileBrowser(window_t *parent, Rect16 rect, const char *media_SFN_path)
    : AddSuperWindow<window_file_list_t>(parent, rect)
    , gcode_info(GCodeInfo::getInstance()) {

    //set root of the file list
    window_file_list_t::SetRoot(root);
    // initialize the directory
    // here the strncpy is meant to be - need the rest of the buffer zeroed
    strncpy(sfn_path, media_SFN_path, sizeof(sfn_path));
    // ensure null character at the end no matter what
    sfn_path[sizeof(sfn_path) - 1] = '\0';
    // cut by the filename to retain only the directory path
    char *c = strrchr(sfn_path, '/');
    *c = 0; // even if we didn't find the '/', c will point to valid memory
    //check if we are at least in the root directory, if not move to root directory
    if (strstr(sfn_path, root) != sfn_path) {
        strlcpy(sfn_path, root, FILE_PATH_BUFFER_LEN);
    }
    // Moreover - the next characters after c contain the filename, which I want to start my cursor at!
    Load(GuiFileSort::Get(), c + 1, firstVisibleSFN);
    // SetItemIndex(1); // this is automatically done in the window file list
}

void WindowFileBrowser::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    switch (event) {

    case GUI_event_t::CLICK:
        break;
    default:
        SuperWindowEvent(sender, event, param);
        return;
    }

    if (file_selected) {
        return;
    }

    static const char dirUp[] = "..";
    static const char slash = '/';

    bool currentIsFile;
    const char *currentSFN = CurrentSFN(&currentIsFile);

    if (!strcmp(currentSFN, dirUp) && window_file_list_t::IsPathRoot(sfn_path)) {
        file_selected = true;
        event_conversion_union un;
        un.action = event_conversion_union::Action::GoHome;
        GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
        return;
    }

    size_t sfnPathLen = strlen(sfn_path);
    if ((sfnPathLen + strlen(currentSFN) + 1) >= MAXPATHNAMELENGTH) {
        log_error(GUI, "Path too long");
        SuperWindowEvent(sender, event, param);
        return;
    }
    if (!currentIsFile) {                // directory selected
        if (strcmp(currentSFN, dirUp)) { // not same -> not ..
            // append the dir name at the end of sfnPath
            if (sfn_path[sfnPathLen - 1] != slash) {
                sfn_path[sfnPathLen++] = slash;
            }
            strlcpy(sfn_path + sfnPathLen, currentSFN, FILE_PATH_BUFFER_LEN - sfnPathLen);
        } else {
            char *last = strrchr(sfn_path, slash);
            if (last == sfn_path) {
                // reached top level dir - ensure it only contains a slash
                sfn_path[0] = slash;
                sfn_path[1] = 0;
            } else {
                *last = '\0'; // truncate the string after the last "/"
            }
        }

        Load(GuiFileSort::Get(), nullptr, nullptr);

        // @@TODO we want to print the LFN of the dir name, which is very hard to do right now
        // However, the text is not visible on the screen yet...
        // header.SetText(strrchr(sfn_path, '/'));

    } else { // print the file
        file_selected = true;
        if (GetParent()) {
            event_conversion_union un;
            un.action = event_conversion_union::Action::FileSelected;
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
        }
        return;
    }
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
