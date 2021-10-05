import matplotlib.pyplot as plt
import statistics, pathvalidate
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
    plot = {
        "plot": {
            "curves": curves,
            "x_label": str(x_key) if not('x_label' in opt_args) else opt_args['x_label'],
            "y_label": y_label
        }
    }
    if 'title' in opt_args:
        plot['plot']['title'] = opt_args['title']
    if 'default_outfile_pdf_name' in opt_args:
        plot['plot']['default_outfile_pdf_name'] = opt_args['default_outfile_pdf_name']
    if 'xlim' in opt_args:
        plot['plot']['xlim'] = opt_args['xlim']
    if 'ylim' in opt_args:
        plot['plot']['ylim'] = opt_args['ylim']
    return plot

def mk_plots(expr,
             plots_expr,
             x_key = 'x', x_vals = [],
             get_y_val = lambda x_key, x_val: 0.0, y_label = 'y',
             curves_expr = mk_unit(),
             opt_args = {}):
    plots_val = eval(plots_expr)
    plots = []
    for plot_row in plots_val['value']:
        plot_expr = {'value': [plot_row]}
        opt_args = opt_args.copy()
        plot_row_dict = row_to_dictionary(plot_row)
        plot_row_str = ",".join("{}-{}".format(*i) for i in plot_row_dict.items())
        opt_args['title'] = plot_row_str
        opt_args['default_outfile_pdf_name'] = pathvalidate.sanitize_filename(plot_row_str)
        plots += [mk_plot(mk_take_kvp(expr, plot_expr),
                          x_key, x_vals, get_y_val, y_label, curves_expr, opt_args)]
    return plots

def output_plot(plot, show_as_popup = False):
    plotd = plot['plot']
    for curve in plotd['curves']:
        xy_pairs = list(curve['xy_pairs'])
        xs = [xyp['x'] for xyp in xy_pairs]
        ys = [xyp['y'] for xyp in xy_pairs] 
        plt.plot(xs, ys, label = curve['curve_label'])
    plt.xlabel(plotd['x_label'])
    plt.ylabel(plotd['y_label'])
    if 'title' in plotd:
        plt.title(plotd['title'])
    if 'xlim' in plotd:
        plt.xlim(plotd['xlim'])
    if 'ylim' in plotd:
        plt.ylim(plotd['ylim'])
    plt.legend()
    if show_as_popup:
        plt.show()
        return
    results_plot_fname = plotd['default_outfile_pdf_name']
    fname = results_plot_fname + '.pdf'
    with PdfPages(fname) as pdf:
        pdf.savefig()
        plt.close()
        print('Generated ' + fname)
