#!/bin/sh
# Copyright (c) 2019 Broadcom.
# The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
#
# Contributors:
#   Broadcom, Inc. - initial API and implementation

#These are MIT-licensed third party sources
EXCEPTIONS="semanticTokens.ts
protocol.semanticTokens.ts
terse.js"

EXLIST=""
for e in $EXCEPTIONS
do
  EXLIST="$EXLIST -and -not -name $e"
done

SOURCES=$(find "-(" -name *.cpp -or -name *.h -or -name *.js -or -name *.ts -or -name *.yml -or -name CMakeLists.txt "-)" $EXLIST)

RET=0
for f in $SOURCES
do
  head $f | grep "Eclipse Public License 2.0" > /dev/null
  if [ $? -ne 0 ]; then
    RET=1
    echo FAILED: $f
  fi
done

return $RET
