# make the algorithms an argument
# make the metrics an argument
# Re-read the "FelixDB Note" file

import os
import sys
import json
import traceback
import random
import math

import matplotlib
import matplotlib.pyplot as plt
import pprint


def compute_average(datapoints, has_shift):
    if len(datapoints) == 0:
        return 0, 0, 0
    #print datapoints
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
    # TODO: very inefficient, optimize this method
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
    #print 'median', values
    median = values[len(values) / 2] - minimum
    perc95 = values[int(float(len(values)) * .95)] - minimum
    maximum = values[-1] - minimum
    return median, perc95, maximum



def convert_datapoints_to_powers_of_2(datapoints):
    out = {}
    for k, v in datapoints.items():
        out[2 ** int(k)] = v
    print 'convert power of 2'
    pprint.pprint(datapoints)
    pprint.pprint(out)
    print '-' * 64
    return out



def aggregate_datapoints(dirpath, testcases, algorithms, shifts):
    print testcases, algorithms, shifts
    aggregate = {}
    for dirname, dirnames, filenames in os.walk(dirpath):
        # print path to all filenames.
        for filename in filenames:
            basename, ext = os.path.splitext(filename)
            if ext.lower() != '.json': continue

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

                #import pprint

                for data in data_items:
                    #print 'data'
                    #pprint.pprint( data_items )
                    #print 'sub'
                    #pprint.pprint( data)
                    average, variance, stddev = compute_average(data['datapoints'], has_shift)
                    median, perc95, maximum = compute_median(data['datapoints'], has_shift)

                    #if 'aligned' in data['metric']:
                    #    datapoints_converted = convert_datapoints_to_powers_of_2(data['datapoints'])
                    #    average_dummy, variance, stddev_dummy = compute_average(datapoints_converted, has_shift)
                    #    print 'variance'
                    #    pprint.pprint(variance)

                    #print "average %f" % (avg)
                    ia = data['algorithm']
                    im = data['metric']
                    ib = data['parameters_hashmap_string']
                    ia = '%s-%s' % (ia, ib)

                    ii = data['instance']
                    ic = data['cycle']

                    it = data['testcase']
                    ip = data['parameters_testcase_string']
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
        #print 'mean_new %f' % (mean_new)
        #print diff, diff_new

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

     

def plot_robinhood(aggregates):

    colors = {'red': '#cd7058', 'blue': '#599ad3', 'orange': '#f9a65a', 'green': '#66cc66', 'black': '#000000', 'purple': '#990066'}

    font = {'family' : 'normal',
            'weight' : 'normal',
            'size'   : 14}
    matplotlib.rc('font', **font)


    numbering_subplots = ['a', 'b', 'c', 'd', 'e']

    for index_stat, statistic in enumerate(['mean', 'median', 'perc95', 'maximum', 'variance']):
        for index_metric, im in enumerate(aggregates.keys()):
            #if  'probing_sequence_length_search' not in im:
            #    continue 
            #fig = plt.figure((index_stat+1) * 10000 + (index_metric+1) * 100 + index_testcase + 1)
            fig = plt.figure((index_stat+1) * 10000 + (index_metric+1) * 100 + 1)
            legend = None
            for index_testcase, it in enumerate(sorted(aggregates[im].keys())):
                if '0.9' in it: continue
                ax = fig.add_subplot(2, 2, index_testcase+1)
                lines = []
                names = []

                # manually sorting the algorithms based on their names, so that they
                # appear ordered in the legend
                algorithms = range(5)
                for ia in aggregates[im][it].keys():
                    if 'linear' in ia:
                        algorithms[0] = ia
                    elif 'backshift' in ia:
                        algorithms[1] = ia
                    elif 'tombstone' in ia:
                        algorithms[2] = ia
                    elif 'shadow' in ia:
                        algorithms[3] = ia
                    elif 'bitmap' in ia:
                        algorithms[4] = ia

                for ia in algorithms:
                #for ia in sorted(aggregates[im][it].keys()):
                    #if   not any(size_str in ia for size_str in ["nb%s-" % (size,) for size in ['10000']]):
                    #  #or not any(algo in ia for algo in ['linear', 'tombstone', 'backshift']):
                    #    v1 = any(size_str in ia for size_str in ["nb%s-" % (size,) for size in ['10000']])
                    #    v2 = any(algo in ia for algo in ['linear', 'tombstone', 'backshift'])
                    #    print "skip [%s] - %s %s" % (ia, v1, v2)
                    #    continue

                    xs = []
                    ys = []

                    for cycle, stats in sorted(aggregates[im][it][ia].items()):
                        if 'loading' in it:
                            xs.append((cycle * 2.0) / 100.0)
                        else:
                            xs.append(cycle)
                        ys.append(sum(stats[statistic]) / len(stats[statistic]))


                    filters = {
                                'linear':    { 'color': colors['blue'],   'name': 'Linear probing',              'linewidth': 8, 'zorder': 1 },
                                'backshift': { 'color': colors['orange'], 'name': 'Robin Hood (backward shift)', 'linewidth': 6, 'zorder': 2 },
                                'tombstone': { 'color': colors['red'],    'name': 'Robin Hood (tombstone)',      'linewidth': 4.5, 'zorder': 3 },
                                'shadow':    { 'color': colors['green'],  'name': 'Hopscotch (shadow)',          'linewidth': 3, 'zorder': 4 },
                                'bitmap':    { 'color': colors['black'],  'name': 'Hopscotch (bitmap)',          'linewidth': 1.75, 'zorder': 5 },
                              }

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

                    if '10000-' not in ia:
                        continue

                    if '10000a-' in ia:
                        name = name + ' (10k)'
                        style = '-'
                        linewidth = 8
                        color = colors['blue']
                    elif '100000a-' in ia:
                        name = name + ' (100k)'
                        style = '-'
                        linewidth = 6
                        color = colors['orange']
                    elif '1000000a-' in ia:
                        name = name + ' (1M)'
                        style = '-'
                        linewidth = 4
                        color = colors['red']
                    elif '10000000a-' in ia:
                        name = name + ' (10M)'
                        style = '-'
                        linewidth = 2
                        color = '#a3a3a3'
                    elif '100000000a-' in ia:
                        name = name + ' (100M)'
                        style = '-'
                        linewidth = 1
                        color = '#000000'

                    line_current, = ax.plot(xs, ys, style, color=color, linewidth=linewidth, zorder=zorder)
                    names.append(name)
                    lines.append(line_current)


                print "%s | %s | %s" % (statistic, im, it)


                if 'loading' in it:
                    ax.set_xlabel('(%s) Load factor' % numbering_subplots[index_testcase])
                else:
                    ax.set_xlabel('(%s) Iterations' % numbering_subplots[index_testcase])

                if statistic == 'mean':
                    ax.set_ylabel('Mean %s' % im)
                    if True or 'loading' not in it:
                        x1,x2,y1,y2 = plt.axis()
                        plt.axis((x1,x2,0,100))
                        #plt.axis((x1,x2,0,40))
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
                        #plt.axis((x1,x2,0,40))
                elif statistic == 'perc95':
                    ax.set_ylabel('95th percentile of %s' % im)
                    if True or 'loading' not in it:
                        x1,x2,y1,y2 = plt.axis()
                        plt.axis((x1,x2,0,100))
                        #plt.axis((x1,x2,0,70))
                elif statistic == 'maximum':
                    ax.set_ylabel('Maximum %s' % im)
                    if True or 'loading' not in it:
                        x1,x2,y1,y2 = plt.axis()
                        plt.axis((x1,x2,0,180))
                #plt.title('%s of %s over %s' % (statistic, im, it))
                plt.title('Test case: %s' % (it.strip('-')))
                #plt.legend(lines, names, loc='upper left', prop={'size':12})
                if not os.path.isdir('plots'):
                    os.mkdir('plots')
                #fig.set_size_inches(8, 6)
                #fig.set_size_inches(8.9, 6.5)
                #reactive fig.set_size_inches(5, 3.75)
                ax.grid(True)

                if any(metric in im for metric in ['blocks', 'aligned']) and statistic != 'variance':
                    labels=['16 B', '32 B', '64 B', '128 B', '256 B', '512 B', '1 KB', '2 KB', '4 KB', '8 KB', '16 KB', '32 KB', '64 KB', '128 KB']
                    plt.axis((x1,x2,4,4+len(labels)))
                    ax.set_yticks(range(4,4+len(labels)))
                    #plt.legend(lines, names, loc='upper left', prop={'size':1}).set_visible(False)
                    ax.set_yticklabels(labels)

                if index_testcase == 3:
                    legend = plt.legend(lines, names, prop={'size':12}, bbox_to_anchor=(0.2, -0.3))
                else:
                    plt.legend(lines, names).set_visible(False)

                


                #from matplotlib import rcParams
                #rcParams.update({'figure.autolayout': True})

                #reactive fig.subplots_adjust(bottom=0.15, left=0.20)
                #ax.legend().set_visible(False)

                #plt.savefig('plots/%s_%s_%s.png' % (im, statistic, it), dpi=72)
            fig.set_size_inches(10, 7.5)
            plt.tight_layout()
            plt.savefig('plots/%s_%s.png' % (im.lower(), statistic), dpi=72, bbox_extra_artists=(legend,), bbox_inches='tight')




def plot_aggregates(aggregates):
    
    colors = [
                '#33E6D9',
                '#FFA600',
                '#A64B00',
                '#8CCCF2',
                '#ED0000',
                '#A6FF00',
                '#8C19A3',
                '#00AAE6',
                '#5CF22C',
                '#FF6600',
                '#806600',
                '#0057D9'
             ]
    for index_stat, statistic in enumerate(['mean', 'median', 'perc95', 'variance']):
        for index_metric, im in enumerate(aggregates.keys()):
            
            print 'im', im
            #if im != 'probing_sequence_length_search': continue
            for index_testcase, it in enumerate(sorted(aggregates[im].keys())):
                fig = plt.figure((index_stat+1) * 10000 + (index_metric+1) * 100 + index_testcase + 1)
                ax = fig.add_subplot(111)
                lines = []
                names = []



                ia_ref = None
                for ia in sorted(aggregates[im][it].keys()):
                    if 'linear' in ia:
                        ia_ref = ia
                        break
                for ia in sorted(aggregates[im][it].keys()):
                    print 'ia', ia
                    #if ia != 'linear': continue
                    xs = []
                    ys = []
                    stddevs = []
                    xs_green = []
                    ys_green = []
                    details = True
                    p_values = []
                    for key, value in sorted(aggregates[im][it][ia].items()):
                        if not value:
                            value = [0]
                        if ia_ref:
                            value_ref = aggregates[im][it][ia_ref][key][statistic]
                        else:
                            value_ref = None
                        xs.append(key)
                        print 'key', key, 'value', value
                        if statistic == 'mean':
                            s = aggregates[im][it][ia][key]['standard_deviation']
                            stddevs.append( (float(sum(s))/float(len(s))) / 2.0 )

                        if ia_ref != None and ia != ia_ref:
                            p_value = 0 #randomized_paired_sample_t_test(value_ref, value, details)
                            print 't-test ', p_value
                            p_values.append(p_value)
                            if p_value < 0.05:
                                xs_green.append(key)
                                ys_green.append(sum(value[statistic]) / len(value[statistic]))
                        ys.append(sum(value[statistic]) / len(value[statistic]))
                        #if details == False:
                        #    details = True
         
                    print 'plot %s | %s' % (ia, im)
                    if 'linear' in ia:
                        style = '-'
                    elif 'tombstone' in ia:
                        style = '-'
                    else:
                        style = '--'
                    color = colors[index_testcase % len(colors)]
                    if statistic == 'mean':
                        print '*' * 64
                        print 'xs', xs
                        print 'ys', xs
                        print 'stdevs', stddevs
                        line_current = ax.errorbar(xs, ys, yerr=stddevs)#, color=color, linewidth=2)
                        line_current = line_current[0]
                    else:
                        line_current, = ax.plot(xs, ys, style, color=color, linewidth=2)
                    lines.append(line_current)
                    names.append('%s-%s' % (ia, it))
                    if ia != ia_ref:
                        #print "TEST RESULTS"
                        #print p_values
                        #print xs_green
                        #print ys_green
                        ax.plot(xs_green, ys_green, linestyle='None', marker='o', color='g', markersize=3)

                #plt.plot([1,2,3,4], [1,4,9,16], 'ro')
                #plt.axis([0, 6, 0, 20])
                #plt.show()

                #print len(lines), len(names)
                ax.set_xlabel('cycles')
                ax.set_ylabel(im)
                plt.title('%s of %s over %s' % (statistic, im, it))
                plt.legend(lines, names, loc='upper left')
                fig.set_size_inches(8, 6)

                if not os.path.isdir('plots'):
                    os.mkdir('plots')
                plt.savefig('plots/%s_%s_%s.png' % (im, statistic, it), dpi=300)
        #plt.show()

        #pprint.pprint(aggregates)

if __name__=="__main__":
    shifts = ""
    if len(sys.argv) == 5:
        shifts = sys.argv[4]
    agg = aggregate_datapoints(sys.argv[1], sys.argv[2], sys.argv[3], shifts)
    #plot_aggregates(agg)
    plot_robinhood(agg)
    #pprint.pprint(agg)
