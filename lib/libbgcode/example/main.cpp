#include <cstdio>
#include <LibBGCode/convert/convert.hpp>

int main()
{
    FILE dst_file, src_file;
    bgcode::convert::from_binary_to_ascii(src_file, dst_file, true);
    
    return 0;
}