#!/usr/bin/env bash

UNDERLINE='-'
PROGRESS_WIDTH=30

CLEAR_LINE='\033[2K'

NOCOLOR='\033[0m'
RED='\033[0;31m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
LIGHTGRAY='\033[0;37m'
DARKGRAY='\033[1;30m'
LIGHTRED='\033[1;31m'
LIGHTGREEN='\033[1;32m'
YELLOW='\033[1;33m'
LIGHTBLUE='\033[1;34m'
LIGHTPURPLE='\033[1;35m'
LIGHTCYAN='\033[1;36m'
WHITE='\033[1;37m'

files="$(find src/ -type f)"

function replicate() {
  printf "%${1}s" | tr " " "${2}"
}

function progress() {
  local numerator="$1"
  local denominator="$2"
  local current="$3"
  local perc=$((100 * numerator / denominator))
  local filled=$((PROGRESS_WIDTH * numerator / denominator))
  local empty=$((PROGRESS_WIDTH - filled))
  printf "\r${CLEAR_LINE}[$(replicate "${filled}" '#')$(replicate "${empty}" ' ')] %d%s %s" "${perc}" '%' "${current}"
}

function end_progress() {
  echo -ne "\r${CLEAR_LINE}"
}

function count_stats() {
  local files="${1}"
  local file_amt=$(echo $files | wc -w)

  local total_lines=0
  local whitespace_lines=0
  local punctuation_lines=0
  local pragma_lines=0

  local i=0
  for file in $files; do
    progress i file_amt "${file}"
    i=$((i + 1))
    contents="$(cat $file)"
    file_line_amt=$(wc -l <<< "$contents")
    total_lines=$((total_lines + file_line_amt))

    file_whitespace_line_amt=$((grep '^\s*$' | wc -l) <<< "${contents}")
    whitespace_lines=$((whitespace_lines + file_whitespace_line_amt))

    file_punctuation_line_amt=$((awk '/^\s*([#%$£"!^&*{}()[\],.])+\s*$/' | wc -l) <<< "${contents}")
    punctuation_lines=$((punctuation_lines + file_punctuation_line_amt))

    file_pragma_line_amt=$((awk '/^\s*#/' | wc -l) <<< "${contents}")
    pragma_lines=$((pragma_lines + file_pragma_line_amt))
  done
  end_progress
  echo "Total lines: ${total_lines}"
  echo "Whitespace lines: ${whitespace_lines}"
  echo "Punctuation lines: ${punctuation_lines}"
  echo "Pragma lines: ${pragma_lines}"
}

DOUBLE_UNDER="${UNDERLINE}${UNDERLINE}"

function title() {
  echo -e "${RED}# ${1}"
  under="$(sed "s/./${UNDERLINE}/g" <<< "${1}")"
  echo -e "${DOUBLE_UNDER}${under}${NOCOLOR}"
}

title "Tests"
test_files=$(grep 'test' <<< $files)
count_stats "$test_files"

echo

title "Source"
non_test_files=$(grep -v 'test' <<< $files)
count_stats "$non_test_files"
