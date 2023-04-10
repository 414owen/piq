// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#define TERM_ESCAPE "\x1B"

// Regular text
#define BLK TERM_ESCAPE "[0;30m"
#define RED TERM_ESCAPE "[0;31m"
#define GRN TERM_ESCAPE "[0;32m"
#define YEL TERM_ESCAPE "[0;33m"
#define BLU TERM_ESCAPE "[0;34m"
#define MAG TERM_ESCAPE "[0;35m"
#define CYN TERM_ESCAPE "[0;36m"
#define WHT TERM_ESCAPE "[0;37m"

// Regular bold text
#define BBLK TERM_ESCAPE "[1;30m"
#define BRED TERM_ESCAPE "[1;31m"
#define BGRN TERM_ESCAPE "[1;32m"
#define BYEL TERM_ESCAPE "[1;33m"
#define BBLU TERM_ESCAPE "[1;34m"
#define BMAG TERM_ESCAPE "[1;35m"
#define BCYN TERM_ESCAPE "[1;36m"
#define BWHT TERM_ESCAPE "[1;37m"

// Regular underline text
#define UBLK TERM_ESCAPE "[4;30m"
#define URED TERM_ESCAPE "[4;31m"
#define UGRN TERM_ESCAPE "[4;32m"
#define UYEL TERM_ESCAPE "[4;33m"
#define UBLU TERM_ESCAPE "[4;34m"
#define UMAG TERM_ESCAPE "[4;35m"
#define UCYN TERM_ESCAPE "[4;36m"
#define UWHT TERM_ESCAPE "[4;37m"

// Regular background
#define BLKB TERM_ESCAPE "[40m"
#define REDB TERM_ESCAPE "[41m"
#define GRNB TERM_ESCAPE "[42m"
#define YELB TERM_ESCAPE "[43m"
#define BLUB TERM_ESCAPE "[44m"
#define MAGB TERM_ESCAPE "[45m"
#define CYNB TERM_ESCAPE "[46m"
#define WHTB TERM_ESCAPE "[47m"

// High intensty background
#define BLKHB TERM_ESCAPE "[0;100m"
#define REDHB TERM_ESCAPE "[0;101m"
#define GRNHB TERM_ESCAPE "[0;102m"
#define YELHB TERM_ESCAPE "[0;103m"
#define BLUHB TERM_ESCAPE "[0;104m"
#define MAGHB TERM_ESCAPE "[0;105m"
#define CYNHB TERM_ESCAPE "[0;106m"
#define WHTHB TERM_ESCAPE "[0;107m"

// High intensty text
#define HBLK TERM_ESCAPE "[0;90m"
#define HRED TERM_ESCAPE "[0;91m"
#define HGRN TERM_ESCAPE "[0;92m"
#define HYEL TERM_ESCAPE "[0;93m"
#define HBLU TERM_ESCAPE "[0;94m"
#define HMAG TERM_ESCAPE "[0;95m"
#define HCYN TERM_ESCAPE "[0;96m"
#define HWHT TERM_ESCAPE "[0;97m"

// Bold high intensity text
#define BHBLK TERM_ESCAPE "[1;90m"
#define BHRED TERM_ESCAPE "[1;91m"
#define BHGRN TERM_ESCAPE "[1;92m"
#define BHYEL TERM_ESCAPE "[1;93m"
#define BHBLU TERM_ESCAPE "[1;94m"
#define BHMAG TERM_ESCAPE "[1;95m"
#define BHCYN TERM_ESCAPE "[1;96m"
#define BHWHT TERM_ESCAPE "[1;97m"

// Reset
#define RESET TERM_ESCAPE "[0m"
