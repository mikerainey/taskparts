from parameter import *
# Table generation from:
#   https://github.com/astanin/python-tabulate
from tabulate import *

def mk_table(all_results, mk_rows, mk_cols,
             gen_row_str = lambda mk_row:
             "...",
             gen_cell_str = lambda mk_row, mk_col, row:
             "...",
             column_titles = []):
    table = []
    for b in [mk_expr_of_row(r) for r in rows_of(mk_rows)]:
        mk_r = eval(mk_take_kvp(all_results, b))
        cols = []
        for m in [mk_expr_of_row(r) for r in rows_of(mk_cols)]:
            n = str(gen_cell_str(b, m, collapse_rows(rows_of(mk_take_kvp(mk_r, m)))))
            cols += [n]
        row = [ gen_row_str(b) ] + cols
        table += [ row ]
    headers = ['benchmark'] + column_titles
    return tabulate(table, headers, floatfmt=".3f", colalign=('right', 'decimal',))
