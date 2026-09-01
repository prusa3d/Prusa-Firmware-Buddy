# Technical details

## 1. Used standards
- [ISO/IEC 15693-3](https://en.wikipedia.org/wiki/ISO/IEC_15693)
- [NFC Data Exchange Format (NDEF)](https://nfc-forum.org/build/specifications/data-exchange-format-ndef-technical-specification/)
- [Concise Binary Object Representation (CBOR)](https://cbor.io/)

## 2. Notes & recommendations
1. The standard was designed with ICODE SLIX2 320 B tag in mind, but it is meant be compatible with all NFC-V tags.
   - Smaller tags might not fit all features/data the OpenPrintTag standard offers. It is up to the manufacturers to decide what data they want to provide in that case.
1. We recommend to **expand the payload of the NDEF record so that the whole available memory of the NFC tag is used.**
   - The idea is that the factory can fully lock the blocks containing the NDEF headers and would only keep the memory pool open. (Possibly write protecting  part of it as well).
   - Part of the memory pool is auxiliary section, which we want to be able to write to without having to edit other structures/shuffle things around.
1. The meta section allows the manufacturers to configure the payload structure so that it fits their purposes and the used NFC tag specifics:
   1. The auxiliary region can be adjusted or even omitted, based on the NFC tag available memory.
      - It is not specified that the auxiliary region must be after the main region. Auxiliary region can possibly be located **before** the main region, if the meta section says so. We however recommend **putting the main region before the auxiliary region**, so that it can be protected together with the NDEF header.
1. The specification was written in such way so that it wouldn't need explicit versioning.
   - New keys can easily be introduced without breaking backwards compatibility.
   - Deprecated keys will never be reused.
   - If there would be substantial changes to the standard that would break backwards compatibility, a new MIME type will be used for the new format.
1. The standard does not attempt to provide anti-counterfeiting or tag authenticity.
   - A NFC tag can be cloned verbatim, or the contents of the container it is attached to can be changed, and no on-tag mechanism can prevent this.
   - For these reasons the standard **does not define on-tag cryptographic signing**, which would cost scarce tag memory while providing marginal benefit.
   - Additional authenticity measures (for example looking up the tag UID against a database) can be employed by the manufacturers, but are out of scope of this specification.

## 3. Tag operations
The OpenPrintTag standard defines these types of operations with the tag:

### 3.1 Read operation
Read operation does not change any data on the tag and is not subject to any limitations.

1. Regions MAY be padded with invalid/leftover data after the section CBOR object. Readers SHALL ignore this data.
1. Example of a typical read operation (results of the steps can be cached to speed up subsequent operations):
   1. Locate the NDEF TLV record.
   1. Locate the OPT NDEF record within the NDEF TLV.
   1. Read the meta section (one CBOR object immediately at the beginning of the NDEF record).
   1. Determine positions of the main and optional auxiliary regions from the meta section data.
   1. Read through the required region and pick out the required fields.

### 3.2 Update operation
Update operation updates selected fields in the auxiliary (or main) region.

1. The operation SHOULD NOT write to any blocks outside the updated region.
   1. *Note: Writes to the tag outside of the auxiliary region MAY be password-protected or completely locked (see the Write protection section). Auxiliary region data update can fail if the operation attempts to rewrite the entire tag.*
1. The operation SHALL NOT change any data outside of the updated region.
   1. The operation SHALL NOT change the position of the OpenPrintTag NDEF record within the tag.
   1. The operation SHALL NOT change positions or sizes of the regions.
   1. The operation SHALL NOT change any data in the metadata section.
1. The operation SHALL NOT change data of any unrelated fields.
   1. Fields with unknown keys or invalid data SHALL be preserved.
   1. Modifications that do not change the CBOR canonical representation (for example reordering map items) are permitted, but not recommended.
1. The operation MAY intentionally remove fields.
1. If the size of the section (CBOR object) within the region shrinks, the leftover data CAN be left as-is. Readers SHALL ignore any padding data past the section CBOR object.
1. Example of a typical update operation (changing one field value):
   1. Parse the TLV record, NDEF record and metadata section in the same way as in the read operation.
   1. Read the updated region to the memory.
   1. Generate new data for the region with the updated field.
   1. Determine the smallest memory region that differs between the old and new data.
   1. Only write the differing region to the tag.

### 3.3 (Re)initialization operation
Initialization rewrites the whole tag and is only intended to be done once.

Initializing a tag requires knowledge about the specific tag model and general managerial decisions:
   1. Unlocking the tag if it has been locked.
   1. Determining the allocation size of the OPT NDEF record.
   1. Determining the auxiliary region size and position.
      1. Auxiliary region SHALL be aligned to the tag memory block boundaries (typically 4B). Main region is generally not intended to be updated, so its alignment is not required.
   1. Configuring relevant NFC tag register:
      1. **AFI**: We recommend setting the AFI register to 0 and locking/password protecting it.
      1. **DSFID**: The DSFID register is reserved for future use. It shall be set to 0 and locked/password protected.
      1. **EAS**: Implementing EAS is up to each manufacturer discretion. We again recommend locking/password protecting it.
   1. Configuring the write protection of the tag (see the Write protection section below).

### 3.4 Reusing the tag
Reusing the tag for a completely different material is generally considered an update operation, as it only clears both main and auxiliary regions and fills the main region with new data.

The tag can however possibly contain additional NDEF records, for example URI with a link relevant to the original material. In that case, removing the NDEF URI record would be desirable, which would constitute a reinitialization operation.

## 4. Write protection
The OpenPrintTag standard offers the following options also considers ways to prevent the tags from being overwritten by malicious actors.

### 4.1 Protecting the auxiliary region
Auxiliary region cannot be reasonably write-protected, because it is intended to be written to by the printers and that needs to be plug-an-play for user comfort.
Therefore it can contain invalid data when a customer first brings it from the shop (because anyone could have written anything to it).

To remedy this issue, the auxiliary section has the `workgroup` field specified. Devices accessing the auxiliary region SHOULD have a workgroup specified (generated randomly by default, but can be changed by the user so that multiple devices have the same workgroup).
When a tag is detected by the device and the workgroup doesn't match, the device SHOULD offer wiping the auxiliary region. The tag workgroup field SHOULD then be set to the device workgroup (even if the user decided not to wipe the region, to prevent repeated prompts).

As a result, the user should be alerted to wipe the auxiliary region on first usage of the filament and then, because the workgroup is the same, there should be no further obstructions.

### 4.2 Protecting the rest of the tag
The rest of the tag (NDEF header, meta region, main region) can be protected more strongly, because it is not expected to be changed regularily (just for refills or reusing the tag). The form of protection is indicated by the `write_protection` field in the main section.

* The locking mechanisms work on the block level, so we recommend **aligning the regions with the NFC tag blocks**.

#### 4.2.1 `write_protection` items
{{ enum_table("write_protection_enum") }}

#### 4.2.2 `irreversible` protection
The irreversible locking can be achiaved using the `LOCK BLOCK` command on the whole tag (except the auxiliary region). We do not recommend this, as it prevents reuse of the NFC tag.

#### 4.2.3 `protect_page_unlockable` protection (SLIX2-specific)
Using `protect_page_unlockable` is the recommended way of protecting the tag. It is based on the `PROTECT PAGE` command, which is a SLIX2-specific command.

1. The tag is write-protected using the `PROTECT PAGE` page command.
   - The protection SHOULD be marked in byte 1 of the Capability Container (CC)
1. The `PROTECT PAGE` on SLIX2 splits the tag memory into two arbitrarily-sized parts and offers setting up different protections on each. So the tag should be arranged in such way that:
   1. The NDEF header and meta and main regions are together on the first part with write protection enabled.
   1. The auxiliary region is on the second part with write protection disabled.
1. The main region can be written to when a correct write password is set using `SET PASSWORD`.
1. The main region can be fully unlocked by a correct `PROTECT_PAGE` command (resetting the protection flags) when the correct write password is set.
1. The password SHALL be randomly generated for each package instance.
1. The password SHOULD be located somewhere on the container, in such way that it is not accessible without unpacking the container.
   - We recommend encoding the password in a QR code in an URL. In this case, a brand-specific pattern matching regex should be defined how to parse the password from the URL.
   - The password can also be in the form of a user-readable text.
