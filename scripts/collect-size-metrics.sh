#!/bin/sh

jq \
      --null-input \
      --arg mainsize "$(stat -c '%s' main)" \
      --arg testsize "$(stat -c '%s' test)" \
  '[
    {name: "Main filesize", unit: "bytes", value: $mainsize},
    {name: "Test filesize", unit: "bytes", value: $testsize}
  ]'
