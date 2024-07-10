#include <catch2/catch.hpp>

#include <dynamic_index_mapping.hpp>

enum Item {
    item1,
    item2,
    item3,
    item4,

    section1,
    section2,
};

template <Item item, size_t pos_in_section = 0, typename M>
bool check_mapping(const M &mapping, size_t index) {
    CAPTURE(item, pos_in_section, index);
    CHECK(mapping.template to_index<item>(pos_in_section) == index);

    const auto from_index = mapping.from_index(index);
    CHECK(from_index.item == item);
    CHECK(from_index.pos_in_section == pos_in_section);

    return (mapping.template to_index<item>(pos_in_section) == index) && (mapping.from_index(index) == typename M::FromIndexResult { item, pos_in_section });
}

TEST_CASE("dynamic_index_mapping::basic_tests") {
    SECTION("Only static items") {
        static constexpr auto mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({ item1, item2, item3, item4 });
        DynamicIndexMapping<mapping_items> mapping;

        size_t i = 0;
        CHECK(check_mapping<item1>(mapping, i++));
        CHECK(check_mapping<item2>(mapping, i++));
        CHECK(check_mapping<item3>(mapping, i++));
        CHECK(check_mapping<item4>(mapping, i++));
        CHECK(mapping.total_item_count() == i);
    }

    SECTION("Single dynamic item") {
        static constexpr auto mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({ item1, item2, { section1, DynamicIndexMappingType::dynamic_section }, item3, item4 });
        DynamicIndexMapping<mapping_items> mapping;

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<item2>(mapping, i++));
            CHECK(check_mapping<section1>(mapping, i++));
            CHECK(check_mapping<item3>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }

        mapping.set_section_size<section1>(2);

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<item2>(mapping, i++));
            CHECK(check_mapping<section1>(mapping, i++));
            CHECK(check_mapping<section1, 1>(mapping, i++));
            CHECK(check_mapping<item3>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }

        mapping.set_section_size<section1>(0);

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<item2>(mapping, i++));
            CHECK(check_mapping<item3>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }
    }

    SECTION("Try everything") {
        static constexpr auto mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
            item1,
            { item2, DynamicIndexMappingType::optional_item },
            { section1, DynamicIndexMappingType::dynamic_section },
            { item3, DynamicIndexMappingType::static_section, 3 },
            { section2, DynamicIndexMappingType::dynamic_section, 2 },
            item4,
        });
        DynamicIndexMapping<mapping_items> mapping;

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<item2>(mapping, i++));
            CHECK(check_mapping<section1>(mapping, i++));
            CHECK(check_mapping<item3, 0>(mapping, i++));
            CHECK(check_mapping<item3, 1>(mapping, i++));
            CHECK(check_mapping<item3, 2>(mapping, i++));
            CHECK(check_mapping<section2>(mapping, i++));
            CHECK(check_mapping<section2, 1>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }

        mapping.set_item_enabled<item2>(false);

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<section1>(mapping, i++));
            CHECK(check_mapping<item3, 0>(mapping, i++));
            CHECK(check_mapping<item3, 1>(mapping, i++));
            CHECK(check_mapping<item3, 2>(mapping, i++));
            CHECK(check_mapping<section2>(mapping, i++));
            CHECK(check_mapping<section2, 1>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }

        mapping.set_section_size<section1>(3);

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<section1>(mapping, i++));
            CHECK(check_mapping<section1, 1>(mapping, i++));
            CHECK(check_mapping<section1, 2>(mapping, i++));
            CHECK(check_mapping<item3, 0>(mapping, i++));
            CHECK(check_mapping<item3, 1>(mapping, i++));
            CHECK(check_mapping<item3, 2>(mapping, i++));
            CHECK(check_mapping<section2>(mapping, i++));
            CHECK(check_mapping<section2, 1>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }

        mapping.set_section_size<section2>(4);

        {
            size_t i = 0;
            CHECK(check_mapping<item1>(mapping, i++));
            CHECK(check_mapping<section1>(mapping, i++));
            CHECK(check_mapping<section1, 1>(mapping, i++));
            CHECK(check_mapping<section1, 2>(mapping, i++));
            CHECK(check_mapping<item3, 0>(mapping, i++));
            CHECK(check_mapping<item3, 1>(mapping, i++));
            CHECK(check_mapping<item3, 2>(mapping, i++));
            CHECK(check_mapping<section2>(mapping, i++));
            CHECK(check_mapping<section2, 1>(mapping, i++));
            CHECK(check_mapping<section2, 2>(mapping, i++));
            CHECK(check_mapping<section2, 3>(mapping, i++));
            CHECK(check_mapping<item4>(mapping, i++));
            CHECK(mapping.total_item_count() == i);
        }
    }
}
