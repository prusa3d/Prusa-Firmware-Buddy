# Material certifications
Material certifications allow expressing what certificates a material has.

The certifications are stored as an enum on the tag to reduce memory usage and promote standard encoding. If there is a certification missing in the enum, please create a PR or file an issue in the [OpenPrintTag Architecture repository](//github.com/OpenPrintTag/openprinttag-architecture).

{{ enum_table("material_certifications_enum", material_certification_columns) }}
