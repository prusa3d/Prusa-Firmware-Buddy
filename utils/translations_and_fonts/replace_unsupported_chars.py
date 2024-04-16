import os
import sys


def replace_characters(file_path):
    # Define the mapping of characters to be replaced
    replacements = {
        '’': "'",  # RIGHT SINGLE QUOTATION MARK
        '…': '...',  # HORIZONTAL ELLIPSIS
        '\u00A0': ' ',  # NO BREAK SPACE
        'µ': 'u'  # MICRO SIGN
    }

    # Read the contents of the file
    with open(file_path, 'r', encoding='utf-8') as file:
        file_content = file.read()

    # Perform replacements
    for old_char, new_char in replacements.items():
        file_content = file_content.replace(old_char, new_char)

    # Write the modified content back to the file
    with open(file_path, 'w', encoding='utf-8') as file:
        file.write(file_content)


def process_files_in_directory(directory):
    # Iterate over each file in the directory
    for file_name in os.listdir(directory):
        # Get the full path of the file
        file_path = os.path.join(directory, file_name)

        # Check if the item is a file and has the .po extension
        if os.path.isfile(file_path) and file_path.endswith('.po'):
            # Replace characters in the file
            replace_characters(file_path)

        # Check if the item is a directory
        elif os.path.isdir(file_path):
            # Recursively process files in subdirectories
            process_files_in_directory(file_path)


def main(folder_path):
    # Check if the provided folder path exists
    if not os.path.exists(folder_path):
        print("Folder does not exist.")
        return

    # Process files in the specified directory and its subdirectories
    process_files_in_directory(folder_path)

    print("Character replacements completed successfully.")


if __name__ == "__main__":
    # Check if a folder path is provided as an argument
    if len(sys.argv) != 2:
        print("Usage: python replace_unsupported_chars.py <folder_path>")
    else:
        folder_path = sys.argv[1]
        main(folder_path)
