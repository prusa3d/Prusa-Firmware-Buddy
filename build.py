import os
import subprocess


def get_version():
    """Reads the first line of version.txt to get the current version."""
    if os.path.exists("version.txt"):
        with open("version.txt", "r", encoding="utf-8") as f:
            return f.readline().strip()
    return "unknown"


def run_build(preset):
    """Executes the python build utility with specified preset."""
    cmd = [
        "python", "utils/build.py", "--bootloader", "empty", "--preset",
        preset, "--final"
    ]
    print(f"Running build for preset: {preset}")
    subprocess.run(cmd, check=True)


def safe_move(src, dst):
    """Moves a file safely, emulating 'move /Y' by overwriting if destination exists."""
    if os.path.exists(src):
        if os.path.exists(dst):
            os.remove(dst)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        os.rename(src, dst)
        print(f"Moved: {src} -> {dst}")
    else:
        print(f"Warning: Source file not found: {src}")


def main():
    version = get_version()
    languages = ["cs"]  #, "de", "es", "fr", "it", "pl", "jp", "uk"]

    # -------------------------------------------------------------------------
    # MINI
    # -------------------------------------------------------------------------
    for lang in languages:
        preset = f"mini"
        run_build(preset)

        src = os.path.join("build", f"{preset}_release_emptyboot",
                           "firmware.bbf")
        dst = os.path.join("build", f"Snake_MINI_en-{lang}_{version}.bbf")

        safe_move(src, dst)

    # # -------------------------------------------------------------------------
    # # coreXY
    # # -------------------------------------------------------------------------
    # for lang in languages:
    #     preset = f"minixy-en-{lang}"
    #     run_build(preset)

    #     src = os.path.join("build", f"{preset}_release_emptyboot", "firmware.bbf")
    #     dst = os.path.join("build", f"Snake_MINI_coreXY_en-{lang}_{version}.bbf")

    #     safe_move(src, dst)

    # # -------------------------------------------------------------------------
    # # i3 MK3.3
    # # -------------------------------------------------------------------------
    # for lang in languages:
    #     preset = f"mini_i3_mk33-en-{lang}"
    #     run_build(preset)

    #     src = os.path.join("build", f"{preset}_release_emptyboot", "firmware.bbf")
    #     dst = os.path.join("build", f"Snake_MINI_i3_MK33_en-{lang}_{version}.bbf")

    #     safe_move(src, dst)

    # # -------------------------------------------------------------------------
    # # i3 MK3.5 coreXY
    # # -------------------------------------------------------------------------
    # preset_mk35 = "i3xy_mk3.5"
    # run_build(preset_mk35)
    # src_mk35 = os.path.join("build", f"{preset_mk35}_release_emptyboot", "firmware.bbf")
    # dst_mk35 = os.path.join("build", f"Snake_i3_MK3.5_coreXY_{version}.bbf")
    # safe_move(src_mk35, dst_mk35)

    # Replicating the 'pause' command from the batch file
    input("\nPress Enter to continue . . .")


if __name__ == "__main__":
    main()
