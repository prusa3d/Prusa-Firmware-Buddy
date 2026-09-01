import subprocess
import sys
import yaml
import argparse
import os
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("-u", "--update", action="store_true", help="Updates reference files to match the new outputs")

args = parser.parse_args()

tests_dir = Path(__file__).parent
root_dir = tests_dir.parent
utils_dir = root_dir / "utils"

logs_dir = tests_dir / "logs"
logs_dir.mkdir(exist_ok=True)


# Runs nfc_initialize & nfc_update utilities and checks their result, output
def utils_test(input_fn: str | None = None, init_args: list[str] = None, update_args: list[str] = None, info_args: list[str] = None, expect_success: bool = True, expected_info_fn: str | None = None, expected_data_fn: str | None = None):
    all_args = locals()

    class UtilFailure(Exception):
        def __init__(self, code, stderr):
            self.code = code
            self.stderr = stderr

    def run_util(util: str, args: list[str] = [], input=None):
        proc_args = ["python3", str(root_dir / "utils" / f"{util}.py")] + args
        print(f"  Running {proc_args}")
        proc = subprocess.run(args=proc_args, input=input, capture_output=True, check=False, cwd=tests_dir)

        if proc.returncode != 0:
            raise UtilFailure(proc.returncode, proc.stderr)

        return proc.stdout

    def check_util_output(name, data, compare_fn):
        if compare_fn is None:
            return

        print(f"  Comparing {name} against '{compare_fn}'")
        if os.path.isfile(compare_fn):
            with open(compare_fn, "rb") as f:
                expected_data = f.read()
        else:
            expected_data = b""

        if data != expected_data:
            print("! Binary data do not match.", file=sys.stderr)
            if args.update:
                with open(compare_fn, "wb") as f:
                    f.write(data)
            else:
                raise Exception()

    print(f"Testing {all_args}")

    try:
        if input_fn is not None:
            assert init_args is None
            with open(input_fn, "rb") as f:
                binary_output = f.read()

        elif init_args is not None:
            assert input_fn is None
            binary_output = run_util("nfc_initialize", init_args)

        else:
            assert False

        if update_args is not None:
            binary_output = run_util("rec_update", args=update_args, input=binary_output)
            check_util_output("binary data", binary_output, expected_data_fn)

        if info_args is not None:
            info_output = run_util("rec_info", args=info_args, input=binary_output)
            check_util_output("rec_info", info_output, expected_info_fn)

        assert expect_success, "Test should have failed"

    except UtilFailure as e:
        if expect_success:
            print(f"Unexpected return code {e.code}")
            print(e.stderr.decode())
            sys.exit(1)

    print("  Test OK")


# Check that files validate_XX.yaml generate valid tags
for file in tests_dir.glob("validate/*.yaml"):
    utils_test(
        init_args=["--size=312", "--aux-region=32", "--ndef-uri=https://3dtag.org/s/334c54f088"],
        update_args=[str(file)],
        info_args=["--validate", "--extra-required-fields=sample_requirements.yaml"],
    )

# Run encode/decode tests
for file in tests_dir.glob("encode_decode/*_input.yaml"):
    fn_base = str(file).removesuffix("_input.yaml")
    fn_info = f"{fn_base}_info.yaml"

    init_args = ["--size=312", "--aux-region=32"]
    info_args = ["--validate", "--show-all", "--opt-check"]

    with open(file, "r", encoding="utf-8") as f:
        yml = yaml.safe_load(f).get("test_config", {})

        if uri := yml.get("uri"):
            init_args += ["--ndef-uri", uri]

        if val := yml.get("extra_required_fields", "sample_requirements.yaml"):
            info_args += ["--extra-required-fields", val]

        if val := yml.get("tag_uid"):
            info_args += ["--tag-uid", val]

        expect_success = yml.get("expect_success", True)

    utils_test(
        init_args=init_args,
        update_args=[str(file)],
        info_args=info_args,
        expected_info_fn=fn_info,
        expected_data_fn=f"{fn_base}_data.bin",
        expect_success=expect_success,
    )


# Check that basic required fields checking works
utils_test(
    init_args=["--size=312", "--aux-region=32"],
    update_args=["specific/missing_required_fields.yaml"],
    info_args=["--validate"],
    expect_success=False,
)

# Check that -extra-required-fields works
utils_test(
    init_args=["--size=312", "--aux-region=32"],
    update_args=["specific/missing_sample_requirements.yaml"],
    info_args=["--validate", "--extra-required-fields=sample_requirements.yaml"],
    expect_success=False,
)

# Test that we are able to handle unknown fields
if True:
    utils_test(
        init_args=["--size=312", "--aux-region=32"],
        update_args=[f"{tests_dir}/specific/unknown_data.yaml", f"--config-file={tests_dir}/specific/unknown_config.yaml"],
        info_args=["--show-all"],
        expected_info_fn=f"{tests_dir}/specific/unknown_info_1.yaml",
        expected_data_fn=f"{tests_dir}/specific/unknown_data_1.bin",
    )
    utils_test(
        input_fn=f"{tests_dir}/specific/unknown_data_1.bin",
        update_args=[f"{tests_dir}/specific/unknown_update_data.yaml"],
        info_args=["--show-all"],
        expected_info_fn=f"{tests_dir}/specific/unknown_info_2.yaml",
        expected_data_fn=f"{tests_dir}/specific/unknown_data_2.bin",
    )
