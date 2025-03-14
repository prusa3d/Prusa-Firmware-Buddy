import unittest
import yaml
from typing import List, Dict

class TestVerifyPruseErrorContentsForBuddy(unittest.TestCase):
    def verify_item_in_error(self, error: Dict, item_name: str, can_be_empty:bool = False):
        assert item_name in error, f"Missing item {item_name} in error code {error['code']}"
        if not can_be_empty:
            assert len(error[item_name]) > 0, f"Item {item_name} in error code {error['code']} is empty"

    def verify_non_empty_array_item(self, error: Dict, item_name: str, is_optional: bool = False):
        assert item_name in error or is_optional, f"Missing item {item_name} in error code {error['code']}"
        if (is_optional and item_name in error) or not is_optional:
            assert len(error[item_name]) > 0, f"Empty array item {item_name} defined in error code {error['code']}"

    def test_buddy_errors(self):
        errors = []
        with open("./yaml/buddy-error-codes.yaml", "r") as buddy_errors:
            errors = yaml.load(buddy_errors, Loader=yaml.Loader)["Errors"]
        for error in errors:
            assert "code" in error, f"Missing error code code in definition: {error}"
            assert error["code"].startswith("XX"), f"Code {error['code']} is missing XX prefix"
            if "deprecated" in error and error["deprecated"]:
                continue
            self.verify_item_in_error(error, "id")
            self.verify_item_in_error(error, "title")
            self.verify_item_in_error(error, "text")
            self.verify_item_in_error(error, "approved", True)
            self.verify_non_empty_array_item(error, "printers", True)


    def test_mmu_errors(self):
        errors = []
        with open("./yaml/mmu-error-codes.yaml", "r") as mmu_errors:
            errors = yaml.load(mmu_errors, Loader=yaml.Loader)["Errors"]
        for error in errors:
            assert "code" in error, f"Missing error code code in definition: {error}"
            assert error["code"].startswith("04"), f"Code {error['code']} is missing 04 prefix"
            if "deprecated" in error and error["deprecated"]:
                continue
            self.verify_item_in_error(error, "id")
            self.verify_item_in_error(error, "title")
            self.verify_item_in_error(error, "text")
            self.verify_item_in_error(error, "type")
            self.verify_item_in_error(error, "approved", True)
            self.verify_non_empty_array_item(error, "action")

if __name__ == "__main__":
    unittest.main()
