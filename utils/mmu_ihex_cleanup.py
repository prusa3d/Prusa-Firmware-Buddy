import sys

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python remove_comments.py input.hex output.hex")
        sys.exit(1)

    input_file = open(sys.argv[1], "r")
    output_file = open(sys.argv[2], "w")

    for line in input_file:
        # Remove empty and comment lines, because objcopy cannot read them
        if not line.strip() or line.startswith(";"):
            continue

        output_file.write(line)
