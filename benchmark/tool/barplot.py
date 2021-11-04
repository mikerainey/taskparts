import matplotlib.pyplot as plt
import statistics, pathvalidate
from matplotlib.backends.backend_pdf import PdfPages
from parameter import *

# x_expr: benchmark
# y_expr: working / stealing / sleeping
def mk_stacked_barplot(expr,
                       x_expr,
                       y_expr,
                       get_y_vals = lambda expr, x_expr, y_val: 0.0):
    r_values = []
    val = eval(expr)
    y_val = eval(y_expr)
    x_val = eval(x_expr)
    for y_row in genfunc_expr_by_row(y_val):
        y_res = get_y_vals(val, x_val, y_row)
        r_values += [{'y_row': y_row, 'res': y_res}]
    barplot = {
        'barplot': {
            'x_expr': eval(x_expr),
            'y_expr': y_val,
            'r_values': r_values
        }
    }
    return barplot

def output_barplot(barplot, outfile = 'barplot'):
    fig, ax = plt.subplots()
    bp = barplot['barplot']
    x_labels = []
    for x in bp['x_expr']['value']:
        l = "-".join("{}={}".format(*i) for i in row_to_dictionary(x).items())
        x_labels += [l]
    ys = {}
    bot = [0.0 for l in x_labels]
    for r in bp['r_values']:
        l = r['y_row']['value'][0][0]['key']
        v = r['res']
        if bot == []:
            ax.bar(x_labels, v, width=0.35, label=l)
        else:
            ax.bar(x_labels, v, width=0.35, label=l, bottom = bot)
        bot = [b + v for b, v in list(zip(v, bot))]
    ax.set_ylabel('y')
    ax.set_title('title')
    ax.legend()
    plt.xticks(rotation=90)
    fname = outfile + '.pdf'
    with PdfPages(fname) as pdf:
        pdf.savefig()
        plt.close()
        print('Generated ' + fname)
        #plt.show()

