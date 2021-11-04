from barplot import *
from parameter import *
from benchmark import *
from taskparts import *
import statistics

def mean(xs):
    if xs == []:
        return 0.0
    return statistics.mean([float(x) for x in xs])


mk_b1 = mk_cross_sequence([mk_parameter('benchmark', 'b1'),
                           mk_parameter('working', 10.0),
                           mk_parameter('stealing', 3.0),
                           mk_parameter('sleeping', 5.3)])

mk_b21 = mk_cross_sequence([mk_parameter('benchmark', 'b2'),
                            mk_parameter('working', 8.0),
                            mk_parameter('stealing', 2.0),
                            mk_parameter('sleeping', 1.0)])

mk_b22 = mk_cross_sequence([mk_parameter('benchmark', 'b2'),
                            mk_parameter('working', 8.1),
                            mk_parameter('stealing', 2.1),
                            mk_parameter('sleeping', 1.1)])
mk_b2 = mk_append_sequence([mk_b21, mk_b22])

expr = mk_append_sequence([mk_b1, mk_b2])

ppj(eval(expr))

def get_y_vals(expr, x_expr, y_val):
    rs = []
    for x_row in genfunc_expr_by_row(x_expr):
        x_val = eval(mk_take_kvp(expr, x_row))
        for y_row in genfunc_expr_by_row(y_val):
            y_key = y_row['value'][0][0]['key']
            ys = select_from_expr_by_key(x_val, y_key)
            y = 0.0
            if ys != []:
                y = mean(ys)
            rs += [y]
    return rs

bp = mk_stacked_barplot(expr,
                        mk_append_sequence([mk_parameter('benchmark', 'b1'),
                                            mk_parameter('benchmark', 'b2')]),
                        mk_append_sequence([mk_parameter('working', 'number'),
                                            mk_parameter('stealing', 'number'),
                                            mk_parameter('sleeping', 'number')]),
                        get_y_vals = get_y_vals)

output_barplot(bp)

# labels = ['G1', 'G2', 'G3', 'G4', 'G5']
# men_means = [20, 35, 30, 35, 27]
# women_means = [25, 32, 34, 20, 25]
# width = 0.35       # the width of the bars: can also be len(x) sequence

# fig, ax = plt.subplots()

# ax.bar(labels, men_means, width, label='Men')
# ax.bar(labels, women_means, width, bottom=men_means,
#        label='Women')

# ax.set_ylabel('Scores')
# ax.set_title('Scores by group and gender')
# ax.legend()

# plt.show()

