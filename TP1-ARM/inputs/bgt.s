.text
movz x11, 8
cmp X11, X12
bgt foo
adds X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X12
bgt bar
adds X3, X0, 10

HLT 0