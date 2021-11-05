import matplotlib.pyplot as plt
import statistics, pathvalidate
import numpy as np
from matplotlib.backends.backend_pdf import PdfPages
#import pandas as pd
from parameter import *

# x_expr: benchmark
# y_expr: working / stealing / sleeping
def mk_stacked_barplot(expr,
                       x_expr,
                       y_expr,
                       get_y_vals = lambda expr, x_expr, y_val: 0.0,
                       opt_args = {}):
    r_values = []
    val = eval(expr)
    y_val = eval(y_expr)
    x_val = eval(x_expr)
    for y_row in genfunc_expr_by_row(y_val):
        y_res = get_y_vals(val, x_val, y_row)
        r_values += [{'y_row': y_row, 'res': y_res}]
    x_axis_label = 'x'
    y_axis_label = 'y'
    title = 'title'
    bp = {
        'barplot': {
            'x_expr': eval(x_expr),
            'y_expr': y_val,
            'r_values': r_values,
            'opt_args': opt_args
        }
    }
    return bp

def output_barplot(barplot, outfile = 'barplot', show_as_popup = False):
    fig, ax = plt.subplots()
    bp = barplot['barplot']
    x_labels = []
    for x in bp['x_expr']['value']:
        l = "-".join("{}={}".format(*i) for i in row_to_dictionary(x).items())
        x_labels += [l]
    nb_x_labels = len(x_labels)
    ind = np.arange(nb_x_labels)
    width = 0.35
    ys = {}
    bot = [0.0 for l in x_labels]
    for r in bp['r_values']:
        l = r['y_row']['value'][0][0]['key']
        v = r['res']
        if bot == []:
            ax.bar(ind, v, width = width, label = l)
        else:
            ax.bar(ind + width, v, width = width, label = l, bottom = bot)
        bot = [b + v for b, v in list(zip(v, bot))]
    if 'x_axis_label' in bp['opt_args']:
        ax.set_xlabel(bp['opt_args']['x_axis_label'])
    if 'y_axis_label' in bp['opt_args']:
        ax.set_ylabel(bp['opt_args']['y_axis_label'])
    if 'title' in bp['opt_args']:
        ax.set_title(bp['opt_args']['title'])
    ax.legend(loc = 'best')
    plt.xticks(ind + width / 2, x_labels)
    if show_as_popup:
        plt.show()
        return
    fname = outfile + '.pdf'
    with PdfPages(fname) as pdf:
        pdf.savefig()
        plt.close()
        print('Generated ' + fname)
        #plt.show()

