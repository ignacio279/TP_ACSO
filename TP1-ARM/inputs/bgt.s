.text
movz X0,8
cmp X0, X1
bgt foo
adds X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X12
bgt bar
adds X3, X0, 10

HLT 0