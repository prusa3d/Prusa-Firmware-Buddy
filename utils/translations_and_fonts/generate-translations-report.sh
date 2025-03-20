#!/bin/bash

# Report untranslated strings in the firmware, to be used in CI.
# This should arguably be rewritten in python, but it works for now.

set -e

CURRENT_DIR=`dirname $0`
ROOT_DIR=`git rev-parse --show-toplevel`
TEMP_DIR=`mktemp --directory`

# generate new .pot file
"${CURRENT_DIR}/generate_pot.sh"
POT_FILE="${ROOT_DIR}/src/lang/po/Prusa-Firmware-Buddy.pot"

REPORT_FILE="${TEMP_DIR}/report"

# for each existing .po file
find "${ROOT_DIR}/src/lang/po/" -name "*.po" |while read PO_FILE; do
    # merge new .pot file with existing .po file
    MERGED_FILE="${TEMP_DIR}/`basename ${PO_FILE}`.merged"
    msgmerge --quiet --no-fuzzy-matching --output-file="${MERGED_FILE}" "${PO_FILE}" "${POT_FILE}"

    # find untranslated strings in the merged file
    UNTRANSLATED_FILE="${TEMP_DIR}/`basename ${PO_FILE}`.untranslated"
    msgattrib --untranslated --sort-output --no-wrap "${MERGED_FILE}" > "${UNTRANSLATED_FILE}"

    # append filtered msgid to report file
    grep ^msgid "${UNTRANSLATED_FILE}" >> "${REPORT_FILE}"
done

# report
echo 'untranslated strings begin'
sort ${REPORT_FILE} | uniq |grep -v '^msgid ""$'
echo 'untranslated strings end'

# cleanup
rm -r "${TEMP_DIR}"
git checkout -- "${POT_FILE}"
