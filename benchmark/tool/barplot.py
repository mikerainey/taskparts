import matplotlib.pyplot as plt
import statistics, pathvalidate
from matplotlib.backends.backend_pdf import PdfPages
from parameter import *

# x_expr: benchmark
# y_expr: working / stealing / sleeping
def mk_stacked_barplot(expr,
                       x_expr,
                       y_expr,
                       get_y_val = lambda x_val, y_val: 0.0):
    y_values = []
    val = eval(expr)
    for x_row in genfun_expr_by_row(x_expr):
        x_val = eval(mk_take_kvp(val, x_row))
        y_value = {'x_expr': x_expr, 'y_expr': y_expr, 'y_vals': []}
        for y_row in genfun_expr_by_row(y_expr):
            y = get_y_val(x_val, eval(mk_take_kvp(x_val, y_row)))
            y_value['y_vals'] += [y]
        y_values += [y_value]
    barplot = {
        'barplot': {
            'y_values': y_values
        }
    }
    return barplot                   





labels = ['G1', 'G2', 'G3', 'G4', 'G5']
men_means = [20, 35, 30, 35, 27]
women_means = [25, 32, 34, 20, 25]
width = 0.35       # the width of the bars: can also be len(x) sequence

fig, ax = plt.subplots()

ax.bar(labels, men_means, width, label='Men')
ax.bar(labels, women_means, width, bottom=men_means,
       label='Women')

ax.set_ylabel('Scores')
ax.set_title('Scores by group and gender')
ax.legend()

plt.show()
