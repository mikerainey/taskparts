from flexibench import table as T
from flexibench import benchmark as B
from flexibench import query as Q
from taskparts import *

print(T.rows_of(T.mk_cross2(T.mk_table1('a',1), T.mk_table1('b',2))))
