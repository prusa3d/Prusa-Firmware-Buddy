########################################################################################################################################################################
#
# This file backs up current firmware elf to backup folder.
# It is useful when you have crash dump from some specific version, and you need to find corresponding ELF file to debug it.
#
########################################################################################################################################################################

import shutil, datetime, os, time

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Get the current timestamp in the format "YYYY-MM-DD_HH-MM-SS"
timestamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

# The source file and destination folder
src_file = "build-vscode-buddy/firmware"
dst_folder = "build-vscode-buddy/debug_elf_backups"

# Check if the directory already exists
if not os.path.exists(dst_folder):
    os.makedirs(dst_folder)

# The new name for the copied file
dst_file = f"{dst_folder}/firmware_{timestamp}.elf"

# Copy the file and add timestamp to the name
shutil.copy2(src_file, dst_file)

print(f"Back up current ELF to {dst_file}")

## ERASE older files then 3 days, to avoid this taking up way to much space

# The current time in seconds since the epoch
now = time.time()

# The age threshold for files to delete (3 days in seconds)
age_threshold = 3 * 24 * 60 * 60

# List the files in the dst_folder
for file in os.listdir(dst_folder):
    # Get the full path of the file
    file_path = os.path.join(dst_folder, file)

    # Check if the file is a regular file and if its age exceeds the threshold
    if os.path.isfile(
            file_path) and now - os.path.getctime(file_path) > age_threshold:
        os.remove(file_path)
