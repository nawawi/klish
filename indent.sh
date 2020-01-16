#!/bin/sh
opts=""

# Blank lines
opts="$opts -bad -bap -nsob"

# Statements
opts="$opts -br -ce -cdw -ss -ncs -npcs -saf -sai -saw -nprs -cli0 -cbi0"

# Declarations
opts="$opts -di1 -nbc -npsl -brs -brf"

# Indentation
opts="$opts -nlp -i8 -ci8 -ip0 -ts8 -il1"

# Breaking long lines
opts="$opts -nbbo -hnl -l80"

indent $opts "$@"
