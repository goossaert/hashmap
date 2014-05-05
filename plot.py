# make the algorithms an argument
# make the metrics an argument

import os
import sys
import json
import traceback
import random
import math

import matplotlib
import matplotlib.pyplot as plt
import pprint

colors = {'red': '#cd7058', 'blue': '#599ad3', 'orange': '#f9a65a', 'green': '#66cc66', 'black': '#000000', 'purple': '#990066'}
numbering_subplots = ['a', 'b', 'c', 'd', 'e', 'f']


def compute_average(datapoints, has_shift):
    if len(datapoints) == 0:
        return 0, 0, 0
    num_freq = 0
    sum_metric = 0
    if not has_shift:
        minimum = 0
    else:
        minimum = None

    for key, value in datapoints.iteritems():
        occurrence = float(key)
        frequency = float(value)
        num_freq += frequency
        sum_metric += frequency * occurrence
        if has_shift:
            if minimum is None or occurrence < minimum:
                minimum = occurrence

    if num_freq <= 1:
        return 0, 0, 0

    mean = float(sum_metric) / float(num_freq)

    sum_metric_squared = 0
    for key, value in datapoints.iteritems():
        occurrence = float(key)
        frequency = float(value)
        sum_metric_squared += frequency * (occurrence - mean) * (occurrence - mean)

    variance = float(sum_metric_squared) / float(num_freq - 1)
    standard_deviation = math.sqrt(variance)
    return mean - minimum, variance, standard_deviation


def compute_median(datapoints, has_shift):
    # TODO: very inefficient, could optimize this method
    if len(datapoints) == 0:
        return 0, 0, 0
    values = []
    minimum = None
    for key, value in datapoints.iteritems():
        occurrence = float(key)
        frequency = float(value)
        for i in range(int(frequency)):
            values.append(occurrence)
        if has_shift:
            if minimum is None or occurrence < minimum:
                minimum = occurrence

    if not has_shift: minimum = 0
    values = sorted(values)
    median = values[len(values) / 2] - minimum
    perc95 = values[int(float(len(values)) * .95)] - minimum
    maximum = values[-1] - minimum
    return median, perc95, maximum


def aggregate_datapoints(dirpath_data, testcases, algorithms, shifts):
    print testcases, algorithms, shifts
    aggregate = {}
    for dirname, dirnames, filenames in os.walk(dirpath_data):
        for filename in filenames:
            basename, ext = os.path.splitext(filename)
            if ext.lower() != '.json': continue
            if '50000000' in filename: continue

            if testcases != 'all' and not any(filename.startswith(testcase) for testcase in testcases.split(',')):
                print 'skipping ' + filename
                continue

            if algorithms != 'all' and not any(algorithm in filename for algorithm in algorithms.split(',')):
                print 'skipping ' + filename
                continue

            try:
                filepath = os.path.join(dirname, filename)
                print "Reading file [%s]" % (filepath,)
                f = open(filepath, 'r')
                text = f.read()
                data_items = json.loads(text)
                f.close()
                has_shift = shifts and any(shift in filename for shift in shifts.split(','))
                if not isinstance(data_items, list):
                    data_items = [data_items]

                for data in data_items:
                    average, variance, stddev = compute_average(data['datapoints'], has_shift)
                    median, perc95, maximum = compute_median(data['datapoints'], has_shift)

                    ia = data['algorithm']
                    im = data['metric']
                    ib = data['parameters_hashmap_string']
                    ia = '%s-%s' % (ia, ib)

                    ii = data['instance']
                    ic = data['cycle']

                    it = data['testcase']
                    ip = data['parameters_testcase_string']
                    if '75' in ip:
                        print "before", ip
                        ip = ip.replace('lfm0.75', 'lfm0.80')
                        print "after", ip
                    it = '%s-%s' % (it, ip)
                    if im not in aggregate:
                        aggregate[im] = {}
                    if it not in aggregate[im]:
                        aggregate[im][it] = {}
                    if ia not in aggregate[im][it]:
                        aggregate[im][it][ia] = {}
                    if ic not in aggregate[im][it][ia]:
                        aggregate[im][it][ia][ic] = {}

                    for m in ['mean', 'median', 'perc95', 'standard_deviation', 'variance', 'maximum']:
                        if m not in aggregate[im][it][ia][ic]:
                            aggregate[im][it][ia][ic][m] = []

                    aggregate[im][it][ia][ic]['mean'].append(average)
                    aggregate[im][it][ia][ic]['standard_deviation'].append(stddev)
                    aggregate[im][it][ia][ic]['variance'].append(variance)
                    aggregate[im][it][ia][ic]['median'].append(median)
                    aggregate[im][it][ia][ic]['perc95'].append(perc95)
                    aggregate[im][it][ia][ic]['maximum'].append(maximum)
            except:
                print 'Crashed at file: [%s/%s]' % (dirname, filename)
                print traceback.print_exc()
                sys.exit(1)

    return aggregate 




def randomized_paired_sample_t_test(reference, candidate, details):
    num_items = len(reference)
    random.seed(None)
    population = []
    print 'ref cand', reference, candidate

    diff = []
    for i in range(num_items):
        diff.append(reference[i] - candidate[i])


    num_population = 10240
    for k in range(num_population):
        diff_new = []
        for i in range(num_items):
            sign = -1 if random.random() < 0.5 else 1
            diff_new.append(diff[i] * sign)

        mean_new = float(sum(diff_new)) / float(num_items)
        population.append(mean_new)

    count_passed = 0
    mean = sum(diff) / num_items
    population = sorted(population)

    for mean_current in population:
        if (mean > 0 and mean <= mean_current) or (mean < 0 and mean < mean_currrent):
            break
        count_passed += 1

    if mean > 0:
        count_passed = num_population - count_passed

    if False and details:
        print "*" * 64
        print "*" * 64
        print "details"
        print "population", population[0], population[1], population[-2], population[-1]
        print "mean", mean
        print "count_passed: %f" % (float(count_passed),)
        print "num_pop %f" % (float(num_population), )

    p_value = float(count_passed) / float(num_population)
    print "passed: %f" % (p_value,)
    return p_value



def add_curve_to_plot(ax, aggregates, im, it, index_testcase, statistic, algorithms_ordering, filters, numbering_subplot, includes):
    names = []
    lines = []
    font = {'family' : 'normal',
            'weight' : 'normal',
            'size'   : 14}
    matplotlib.rc('font', **font)

    algorithms = [None] * 5
    for ia in aggregates[im][it].keys():
        for pattern in algorithms_ordering.keys():
            if pattern in ia:
                order = algorithms_ordering[pattern]['order']
                algorithms[order] = ia

    for ia in algorithms:
        if ia is None: continue
        print "Generating curve for: stats:%s | metric:%s | testcase:%s | algorithm:%s" % (statistic, im, it, ia)

        xs = []
        ys = []

        for cycle, stats in sorted(aggregates[im][it][ia].items()):
            if 'loading' in it:
                xs.append((cycle * 2.0) / 100.0)
            else:
                xs.append(cycle)
            ys.append(sum(stats[statistic]) / len(stats[statistic]))

        name = '[ERROR: unknown algorithm]'
        color = '#000000'
        linewidth = 3
        zorder = 1
        for k, v in filters.iteritems():
            if k in ia:
                name = filters[k]['name']
                color = filters[k]['color']
                linewidth = filters[k]['linewidth']
                style = '-'
                zorder = filters[k]['zorder']
                break

        if not any(pattern in ia for pattern in includes):
            continue

        line_current, = ax.plot(xs, ys, style, color=color, linewidth=linewidth, zorder=zorder)
        names.append(name)
        lines.append(line_current)

    if 'loading' in it:
        ax.set_xlabel('(%s) Load factor' % numbering_subplot)
    else:
        ax.set_xlabel('(%s) Iterations' % numbering_subplot)

    if statistic == 'mean':
        ax.set_ylabel('Mean %s' % im)
        if True or 'loading' not in it:
            x1,x2,y1,y2 = plt.axis()
            plt.axis((x1,x2,0,100))
    elif statistic == 'variance':
        ax.set_ylabel('Variance of %s' % im)
        if True or 'loading' not in it:
            x1,x2,y1,y2 = plt.axis()
            plt.axis((x1,x2,0,600))
    elif statistic == 'standard_deviation':
        ax.set_ylabel('Standard deviation of %s' % im)
    elif statistic == 'median':
        ax.set_ylabel('Median of %s' % im)
        if True or 'loading' not in it:
            x1,x2,y1,y2 = plt.axis()
            plt.axis((x1,x2,0,100))
    elif statistic == 'perc95':
        ax.set_ylabel('95th percentile of %s' % im)
        if True or 'loading' not in it:
            x1,x2,y1,y2 = plt.axis()
            plt.axis((x1,x2,0,100))
    elif statistic == 'maximum':
        ax.set_ylabel('Maximum %s' % im)
        if True or 'loading' not in it:
            x1,x2,y1,y2 = plt.axis()
            plt.axis((x1,x2,0,180))
    plt.title('Test case: %s' % (it.strip('-')))
    ax.grid(True)

    if any(metric in im for metric in ['blocks', 'aligned']) and statistic != 'variance':
        labels=['16 B', '32 B', '64 B', '128 B', '256 B', '512 B', '1 KB', '2 KB', '4 KB', '8 KB', '16 KB', '32 KB', '64 KB', '128 KB']
        plt.axis((x1,x2,4,4+len(labels)))
        ax.set_yticks(range(4,4+len(labels)))
        ax.set_yticklabels(labels)

    plt.legend(lines, names).set_visible(False)
    return names, lines


     
def plot_algorithms(aggregates):

    for index_stat, statistic in enumerate(['mean', 'median', 'perc95', 'maximum', 'variance']):
        for index_metric, im in enumerate(aggregates.keys()):
            fig = plt.figure((index_stat+1) * 10000 + (index_metric+1) * 100 + 1)
            legend = None
            for index_testcase, it in enumerate(sorted(aggregates[im].keys())):
                ax = fig.add_subplot(2, 2, index_testcase+1)
                lines = []
                names = []

                names_temp, lines_temp = add_curve_to_plot( 
                                    ax=ax,
                                    aggregates=aggregates,
                                    im=im,
                                    it=it,
                                    index_testcase=index_testcase,
                                    statistic=statistic,
                                    algorithms_ordering = {
                                                            'linear': {'order': 0},
                                                            'backshift': {'order': 1},
                                                            'tombstone': {'order': 2},
                                                            'shadow': {'order': 3},
                                                            'bitmap': {'order': 4},
                                                          },
                                    filters = {
                                                'linear':    { 'color': colors['blue'],   'name': 'Linear probing',              'linewidth': 8,    'zorder': 1 },
                                                'backshift': { 'color': colors['orange'], 'name': 'Robin Hood (backward shift)', 'linewidth': 6,    'zorder': 2 },
                                                'tombstone': { 'color': colors['red'],    'name': 'Robin Hood (tombstone)',      'linewidth': 4.5,  'zorder': 3 },
                                                'shadow':    { 'color': colors['green'],  'name': 'Hopscotch (shadow)',          'linewidth': 3,    'zorder': 4 },
                                                'bitmap':    { 'color': colors['black'],  'name': 'Hopscotch (bitmap)',          'linewidth': 1.75, 'zorder': 5 },
                                              },
                                    numbering_subplot=numbering_subplots[index_testcase],
                                    includes=['10000-'],
                                 )

                    
                names.extend(names_temp)
                lines.extend(lines_temp)

            legend = plt.legend(lines, names, prop={'size':12}, bbox_to_anchor=(0.2, -0.3))
            if not os.path.isdir('plots/algorithms'):
                os.mkdir('plots/algorithms')
            fig.set_size_inches(10, 7.5)
            plt.tight_layout()
            plt.savefig('plots/algorithms/%s_%s.png' % (im.lower(), statistic), dpi=72, bbox_extra_artists=(legend,), bbox_inches='tight')





def plot_robinhood(aggregates):
    for index_metric, im in enumerate(aggregates.keys()):
        fig = plt.figure((index_metric+1) * 100 + 1)
        for index_stat, statistic in enumerate(['mean', 'median', 'perc95', 'maximum', 'variance']):
            ax = fig.add_subplot(3, 2, index_stat+1)
            lines = []
            names = []
            for index_testcase, it in enumerate(sorted(aggregates[im].keys())):
                names_temp, lines_temp = add_curve_to_plot( 
                                    ax=ax,
                                    aggregates=aggregates,
                                    im=im,
                                    it=it,
                                    index_testcase=index_testcase,
                                    statistic=statistic,
                                    algorithms_ordering = {
                                                            '10000-': {'order': 0},
                                                            '100000-': {'order': 1},
                                                            '1000000-': {'order': 2},
                                                            '10000000-': {'order': 3},
                                                            '50000000-': {'order': 4},
                                                          },
                                    filters = {
                                                '10000-':     { 'color': colors['blue'],   'name': 'Robin Hood (backward shift, 10k)',  'linewidth': 8, 'zorder': 1 },
                                                '100000-':    { 'color': colors['orange'], 'name': 'Robin Hood (backward shift, 100k)', 'linewidth': 6, 'zorder': 2 },
                                                '1000000-':   { 'color': colors['red'],    'name': 'Robin Hood (backward shift, 1M)',   'linewidth': 4.5, 'zorder': 3 },
                                                '10000000-':  { 'color': colors['green'],  'name': 'Robin Hood (backward shift, 10M)',  'linewidth': 3, 'zorder': 4 },
                                                '50000000-':  { 'color': colors['black'],   'name': 'Robin Hood (backward shift, 50M)', 'linewidth': 1.75, 'zorder': 5 },
                                                '100000000-': { 'color': colors['black'],  'name': 'Robin Hood (backward shift, 100M)', 'linewidth': 1.75, 'zorder': 5 },
                                              },
                                    numbering_subplot=numbering_subplots[index_stat],
                                    includes=['backshift'],
                                 )
                names.extend(names_temp)
                lines.extend(lines_temp)

        legend = plt.legend(lines, names, prop={'size':12}, bbox_to_anchor=(2.10, 0.75))
        fig.set_size_inches(10, 11.25)
        plt.tight_layout()
        if not os.path.isdir('plots/robinhood-backshift'):
            os.mkdir('plots/robinhood-backshift')
        plt.savefig('plots/robinhood-backshift/%s.png' % (im.lower()), dpi=72, bbox_extra_artists=(legend,), bbox_inches='tight')



if __name__=="__main__":
    shifts = ""
    if len(sys.argv) == 5:
        shifts = sys.argv[4]

    agg = aggregate_datapoints(dirpath_data=sys.argv[1],
                               testcases=sys.argv[2],
                               algorithms=sys.argv[3],
                               shifts=shifts)
    plot_algorithms(agg)
    plot_robinhood(agg)
