// RUN: %clang_cc1 -analyze -analyzer-checker=unix.Malloc -analyzer-output=plist -o 1.plist
// RUN: %clang_cc1 -analyze -analyzer-checker=unix.Malloc -analyzer-output=plist -o 2.plist
// RUN: FileCheck -input-file=1.plist %s
// RUN: FileCheck -input-file=2.plist %s

#include <memory.h>
int main( int _argc, char ** _argv )
{
    void * m = malloc( 100 );return 0;
}


// CHECK:  <key>issue_hash</key><string>{{[0-9]+}}</string>
