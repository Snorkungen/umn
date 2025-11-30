# UMN

> The aim is to create a utility program to convert hexadecimal numbers to decimal and vice-versa.

A utility program that understands different representations of numbers and then display the numbers.

```sh
<PROMPT> umn -d 0x1ee7
"0x1ee7" = 7911

<PROMPT> umn -x 7911
"7911" = 0x1ee7 

<PROMPT> umn -d 7911
"7911" = 7911 

<PROMPT> umn -b 7911
"7911" = 0b1111011100111
```

> v026.0 Breaking changes
- simplify the input interface by enforcing options being theeeee first values
- support bitwise expressions
- do not have to worry about operator precedence
```sh
umn [options] [expr1, expr2, exprN]
<PROMPT> umn -db 0x1ee7
10: "0x1EE7"=7911
 2: "0x1EE7"=0b1111011101110

<PROMPT> umn -d 0x1ee7 & 10
10: "0x1ee7 & 10"=7690

<PROMPT> umn -o 8
 8: "8"=010

```