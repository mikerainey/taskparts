import matplotlib.pyplot as plt
import statistics
from matplotlib.backends.backend_pdf import PdfPages
from parameter import *

def mk_plot(expr,
            x_key = 'x', x_vals = [],
            get_y_val = lambda x_key, x_val: 0.0, y_label = 'y',
            curves_expr = mk_unit(),
            opt_args = {}):
    val = eval(expr)
    curves_val = eval(curves_expr)
    curves = []
    for curve_row in curves_val['value']:
        curve_label = ','.join([ str(kvp['key']) + '=' + str(kvp['val']) for kvp in curve_row ])
        xy_pairs = []
        ev = mk_take_kvp(val, {'value': [curve_row]})
        for x_val in x_vals:
            y_expr = eval(mk_take_kvp(ev, mk_parameter(x_key, x_val)))
            y_val = get_y_val(x_key, x_val, y_expr)
            xy_pairs += [{ "x": x_val, "y": y_val } ]
        curves += [{"curve_label": curve_label, "xy_pairs": xy_pairs}]
    return {
        "plot": {
            "curves": curves,
            "x_label": str(x_key) if not('x_label' in opt_args) else opt_args['x_label'],
            "y_label": y_label
        }
    }

# popup plot of results_plot_name == ''
def output_plot(plot, results_plot_name = 'plot.pdf'):
    plotd = plot['plot']
    for curve in plotd['curves']:
        xy_pairs = list(curve['xy_pairs'])
        xs = [xyp['x'] for xyp in xy_pairs]
        ys = [xyp['y'] for xyp in xy_pairs] 
        plt.plot(xs, ys, label = curve['curve_label'])
    plt.xlabel(plotd['x_label'])
    plt.ylabel(plotd['y_label'])
    plt.legend()
    if results_plot_name == '':
        plt.show()
        return    
    with PdfPages(results_plot_name) as pdf:
        pdf.savefig()
        plt.close()
