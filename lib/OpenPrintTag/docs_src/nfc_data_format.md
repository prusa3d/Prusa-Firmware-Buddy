# NFC Data Format Specification

## 1. Used standards
- [ISO/IEC 15693-3 (NFC-V)](https://en.wikipedia.org/wiki/ISO/IEC_15693)
- [NFC Data Exchange Format (NDEF)](https://nfc-forum.org/build/specifications/data-exchange-format-ndef-technical-specification/)
- [Concise Binary Object Representation (CBOR)](https://cbor.io/)

## 2. General structure
<table class="packet-structure">
   <tr>
      <td rowspan=5>CC record<br>(Capability Container)</td>
      <td colspan=8>NDEF TLV</td>
      <td rowspan=5>TLV Terminator</td>
   </tr>
      <tr>
      <td rowspan=4>TLV Header</td>
   </tr>
   <tr>
      <td colspan=6>NDEF record</td>
   </tr>
   <tr>
      <td rowspan=2>NDEF header</td>
      <td>Meta region</td>
      <td colspan=2>Main region</td>
      <td colspan=2>Auxiliary region</td>
   </tr>
   <tr>
      <td>Meta section</td>
      <td>Main section</td>
      <td class="unused">Unused space</td>
      <td>Auxiliary section</td>
      <td class="unused">Unused space</td>
   </tr>
</table>
<style>
   .packet-structure tbody {
      border: 2px solid black;
   }
   .packet-structure td {
      vertical-align: top;
      text-align: center;
      border: 1px solid black;
      border-left: 2px solid black;
      border-right: 2px solid black;
   }
   .packet-structure .unused {
      opacity: 50%;
   }
</style>

1. The top layer of the NFC tag is an NDEF message in a NDEF TLV record.
   - The tag MAY contain other TLV records. The NDEF TLV record doesn't have to be the first TLV record.
1. The message has an **NDEF record** of MIME type **application/vnd.openprinttag**.
   1. The NDEF record MUST NOT be split into multiple NDEF record chunks.
      - Splitting the record would break the "virtual space" of the payload and would complicate implementation.
   1. The NDEF message MAY contain other NDEF records. The material NDEF record doesn't need to be the first NDEF record in the message.
1. The payload of the OpentPrintTag NDEF record consists of:
   1. **Meta section** (CBOR map)
      1. Always at the beginning of the payload.
      1. Contains information about other regions:
         - Region is a part of the payload allocated for the respective section.
         - A section does not have to fill the whole region.
   1. **Main section** (CBOR map)
      1. Positioned at the beginning of the main region.
      1. Intended for static information, not intended to be updated by printers.
         - The only situation where this region needs to be updated would be when the container is being repurposed.
   1. **Auxiliary section** (optional, CBOR map)
      1. Positioned at the beginning of the auxiliary region.
      1. Intended for dynamic information, intended to be updated by the printers.
      1. SHALL have at least 16 B allocated if present. 32 B is recommended.
      1. The root map of the section SHOULD be encoded as an indefinite-length container.

1. Unused space in the sections (outside of the region CBOR) SHALL NOT contain any meaningful working data. It SHOULD be filled with zeroes on tag initialization, but there are no requirements on upkeeping that afterwards. Users CAN update the regions with smaller data, leaving remnants of the original data behind.

## 3. Specification common to all sections

### 3.1 CBOR data representation
1. Data of all sections in the specification are represented as a CBOR map.
   1. Keys of the map are integers. Semantics of the keys are specific to each section.
   1. All data sections SHALL be at most 512 bytes long.
   1. All fields MUST follow this specification. Using custom or vendor-specific keys is not permitted (with the exception described in the Aux Region section).
   1. New keys can be added to the specification at any time, implementations SHALL be able to skip unknown keys, of any type. Unknown fields SHALL NOT be removed when updating a known field (or in any update proces in general) unless explicitly intended.
      1. This applies to unknown items of `enum` and `enum_array` as well, provided that they are represented as integer values in the range of <0, 65535>.
   1. Keys can be deprecated at any time. Deprecated keys will never be reused.
   1. The keys MAY be arbitrarily ordered within the CBOR map. Implementations SHALL support unsorted (non-canonical) CBOR maps.
1. `enum` fields are encoded as an integer, according to the enum field mapping
1. `enum_array` fields are encoded as CBOR arrays of integers, according to the field mapping
1. `timestamp` fields are encoded as UNIX timestamp integers
1. `number` types can be encoded as either unsigned integers (type 0), signed integers (type 1), half floats or floats
1. `bytes` and `uuid` types are encoded as CBOR byte string (type 2)
1. `string` types are encoded as CBOR text string (type 3, UTF-8 is enforced by the CBOR specification)
1. The `X` in the `string:X` or `bytes:X` notation defines maximum permissible length of the data in bytes.
1. `color_rgba` fields are encoded as a CBOR byte string (type 2) with 3 to 4 bytes representing `[R, G, B]` or `[R, G, B, A]` values
1. `color_lab` fields are encoded as a CBOR array of 3 `number` type elements

### 3.2 UUIDs
Some entities referenced in the data can be identified by a [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier). The UUID MAY be explicitly specified through a `XX_uuid` field, however that might not be desirable due to space constraints. As an alternative, the following algorithm defines a way to derive UUIDs from other fields.

#### 3.2.0.1 When reading a UUID
1. First, the program MUST check if an appropriate `XX_uuid` field is present in the data. If so, the program MUST use that UUID.
1. If the field is not present, the program SHOULD derive the UUID using the algorithm described below, and use it.
1. If the derivation algorithm is not implemented or if the fields required for the derivation are not present and cannot be derived, the UUID is considered not defined and the entity CANNOT be uniquely referenced.

#### 3.2.0.2 When preparing/writing main region data
1. The program MAY omit the `XX_uuid` field from the data if the value is equal to the auto-derived UUID (and thus if the reader would return the same result in both cases).
1. Unless guaranteed that a tag is being used for the first time, the program SHOULD assign a new, unique `instance_uuid` (presumably UUIDv4) and include it in the data. This is to prevent `instance_uuid` collisions when a tag is being reused.
   1. If there is a possibility that the same generated data would get written to multiple tags, the program SHOULD NOT include `instance_uuid`, and instead rely on the auto-derivation mechanism that would yield different UUIDs for different tags.

#### 3.2.0.3 Example: Brand renaming
If a brand decides to change name but wants to keep the original `brand_uuid` that was auto-derived from its name, it needs to start adding `brand_uuid` field with the original UUID:
1. `brand_name = Prusament` (present in the data), `brand_uuid = ae5ff34e-298e-50c9-8f77-92a97fb30b09` (not present, can be automatically derived)
1. Brand gets renamed to `Pepament`
1. `brand_name = Pepament` (present in the data), `brand_uuid = ae5ff34e-298e-50c9-8f77-92a97fb30b09` (present in the data)


### 3.2.1 UUID derivation algorithm
UUIDs are derived from the brand-specific IDs using UUIDv5 with the `SHA1` hash, as specified in [RFC 4122, section 4.3](https://datatracker.ietf.org/doc/html/rfc4122#section-4.3), according to the following table.
1. UUIDs are hashed in the binary form.
1. Strings are encoded as UTF-8.
1. Numbers are encoded as decimal strings.
1. `+` represents binary concatenation.
1. NFC tag UID is represented as a bytestream with the MSB being the first byte in the bytestream.
   * **Important:** Various apps/readers report these UIDs in various byte orders, and sometimes as hex strings instead of bytestreams. For NFCV, the UID MUST be a 8 bytes long bytestream with `0xE0` as the **first** byte (SLIX2 then follows with `0x04, 0x01`).

| UUID | Derviation formula | Namespace (`N`) |
| --- | --- | --- |
| `brand_uuid` | `N + brand_name` | `5269dfb7-1559-440a-85be-aba5f3eff2d2` |
| `material_uuid` | `N + brand_uuid + material_name` | `616fc86d-7d99-4953-96c7-46d2836b9be9` |
| `package_uuid` | `N + brand_uuid + gtin` | `6f7d485e-db8d-4979-904e-a231cd6602b2` |
| `instance_uuid` | `N + nfc_tag_uid` | `31062f81-b5bd-4f86-a5f8-46367e841508` |


For example:
{% python %}
import uuid

def generate_uuid(namespace, *args):
   return uuid.uuid5(uuid.UUID(namespace), b"".join(args))

brand_namespace = "5269dfb7-1559-440a-85be-aba5f3eff2d2"
brand_name = "Prusament"
brand_uuid = generate_uuid(brand_namespace, brand_name.encode("utf-8"))
print(f"brand_uuid = {brand_uuid}")

material_namespace = "616fc86d-7d99-4953-96c7-46d2836b9be9"
material_name = "PLA Prusa Galaxy Black"
material_uuid = generate_uuid(material_namespace, brand_uuid.bytes, material_name.encode("utf-8"))
print(f"material_uuid = {material_uuid}")

package_namespace = "6f7d485e-db8d-4979-904e-a231cd6602b2"
gtin = "1234"
package_uuid = generate_uuid(package_namespace, brand_uuid.bytes, gtin.encode("utf-8"))
print(f"package_uuid = {package_uuid}")

instance_namespace = "31062f81-b5bd-4f86-a5f8-46367e841508"
nfc_tag_uid = b"\xE0\x04\x01\x08\x66\x2F\x6F\xBC"
instance_uuid = generate_uuid(instance_namespace, nfc_tag_uid)
print(f"instance_uuid = {instance_uuid}")
{% endpython %}

## 4. Meta section

The meta section allows defining of region offsets (within the NDEF payload) and sizes.
- Main region is always present. If the offset is not specified, the region starts right after the meta section.
- Auxiliary region is optional (although heavily recommended). Its presence is indicated by the `aux_region_offset` field.

### 4.1 Field list
{{ fields_table("meta_fields") }}

## 5. Main section
The main section contains material information that does not change during the package instance lifetime.
   - This section can possibly be locked by the manufacturer.

### 5.1 Field list
{{ fields_table("main_fields", "") }}

#### 5.1.1 Field list (FFF-specific)
{{ fields_table("main_fields", "fff") }}

##### 5.1.1.1 Removed fields (FFF-specific)

|Name|Key|Removal reason|
|:--|:--|:--|
|`filament_diameter`|30|Unit change (`mm` → `µm`) to align with the OpenPrintTag Architecture.<br>Replaced with `filament_diameter_v2`.|
|`min_nozzle_diameter`|33|Unit change (`mm` → `µm`) to align with the OpenPrintTag Architecture.<br>Replaced with `min_nozzle_diameter_v2`.|

#### 5.1.2 Field list (SLA-specific)
{{ fields_table("main_fields", "sla") }}

#### 5.1.3 `material_class` items
{{ enum_table("material_class_enum") }}

#### 5.1.4 `material_type`
See [FFF material types](/fff_material_types)

#### 5.1.5 `tags`
See [Material tags](/material_tags)

#### 5.1.6 `write_protection` items
{{ enum_table("write_protection_enum") }}

## 6. Auxiliary section
The auxiliary section is intended for dynamic data - typically usage tracking.

### 6.1 Field list
{{ fields_table("aux_fields", "") }}

#### 6.1.1 Field list (SLA-specific)
{{ fields_table("aux_fields", "sla") }}

#### 6.1.1 Field list (user data)
{{ fields_table("aux_fields", "user_data") }}

### 6.2 Vendor-specific fields
Vendor-specific fields not specified in this document are permitted for keys specified by the following table. Vendors may contact the specification authority to be assigned a key range for them to use.

| Min key | Max key | Vendor |
| --- | --- | --- |
| 65400 | 65534 | General purpose |
| 65300 | 65400 | Prusa |

#### 6.2.1 General purpose key range
The "General purpose" key range MAY be used by anyone, provided they follow the following rules:
1. Users MUST assign themselves a unique enough `general_purpose_range_user` value. This is done with no central authority.
1. The range MUST be used only by one user at a time, determined by the `general_purpose_range_user` field.
1. Users MUST ensure that the `general_purpose_range_user` field is set to the value assigned to them for any read or write access to the general purpose range.
1. Users MUST delete any general purpose range fields present before changing the value of `general_purpose_range_user`.
