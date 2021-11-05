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
    for b in genfunc_expr_by_row(mk_rows):
        mk_r = eval(mk_take_kvp(all_results, b))
        cols = []
        for m in genfunc_expr_by_row(mk_cols):
            n = str(gen_cell_str(b, m, collapse_rows(rows_of(mk_take_kvp(mk_r, m)))))
            cols += [n]
        row = [ gen_row_str(b) ] + cols
        table += [ row ]
    headers = ['benchmark'] + column_titles
    return tabulate(table, headers, floatfmt=".3f", colalign=('right', 'decimal',))

def mk_simple_table(expr,
                    mk_rows = mk_unit(),
                    mk_cols = mk_unit(),
                    gen_cell = lambda row_expr, col_expr, row:
                    0.0,
                    gen_row_str = human_readable_string_of_expr,
                    row_title = '',
                    gen_col_titles = lambda cols_expr:
                    [human_readable_string_of_expr(e) for e in genfunc_expr_by_row(cols_expr)]):
    row_labels = []
    rows = []
    for row_expr in genfunc_expr_by_row(mk_rows):
        row_vals = eval(mk_take_kvp(expr, row_expr))
        cols = []
        for col_expr in genfunc_expr_by_row(mk_cols):
            n = gen_cell(row_expr, col_expr, collapse_rows(rows_of(mk_take_kvp(row_vals, col_expr))))
            cols += [n]
        rows += [ cols ]
        row_labels += [ gen_row_str(row_expr) ]
    t = {
        'simple_table': {
            'col_titles': gen_col_titles(mk_cols),
            'rows': rows,
            'row_title': row_title,
            'row_labels': row_labels
        }
    }
    return t

def gen_simple_table(table):
    t = table['simple_table']
    rows = [ [rl] + r for r, rl in list(zip(t['rows'], t['row_labels'])) ]
    headers = [ t['row_title'] ] + t['col_titles']
    return tabulate(rows, headers, floatfmt=".3f", colalign=('right', 'decimal',))
