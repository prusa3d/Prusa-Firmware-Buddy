#!/bin/bash

echo "Generate gzip WUI resources";
# gzip needed !
if ! [ -x "$(command -v gzip)" ]; then
	echo 'ERROR - gzip is not installed!' >&2
	exit 1;
fi

# get actual path dir to call this script with "absolute" path
path=$(realpath $0);
pathdir=$(dirname $path);

# check if in some cases old gzip directory still exists
if [ -d "$pathdir/../../lib/WUI/resources/src_local_gzip" ]; then
	rm -r ${pathdir}/../../lib/WUI/resources/src_local_gzip;
fi

# copy resources for gzip
cp -r ${pathdir}/../../lib/WUI/resources/src_local ${pathdir}/../../lib/WUI/resources/src_local_gzip;

# gzip copied folder
gzip -f ${pathdir}/../../lib/WUI/resources/src_local_gzip/*;

# rename `gz` from file name
for file in ${pathdir}/../../lib/WUI/resources/src_local_gzip/*.gz
do
	mv "$file" "${file/.gz/}";
done

# generate C file
${pathdir}/genhtmlc ${pathdir}/../../lib/WUI/resources/src_local_gzip -e -f:${pathdir}/../../lib/WUI/resources/fsdata_wui_local.c;

# delete copied gzip folder
rm -r ${pathdir}/../../lib/WUI/resources/src_local_gzip;
