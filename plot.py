# make the algorithms an argument
# make the metrics an argument
# Re-read the "FelixDB Note" file

import os
import sys
import json
import traceback
import random

import matplotlib.pyplot as plt
import pprint


def compute_average(datapoints, has_shift):
    #print datapoints
    num_freq = 0
    sum_metric = 0
    minimum = None
    for key, value in datapoints.iteritems():
        occurrence = float(key)
        frequency = float(value)
        num_freq += frequency
        sum_metric += frequency * occurrence
        if has_shift:
            if minimum is None or occurrence < minimum:
                minimum = occurrence

    if not has_shift: minimum = 0
    return sum_metric / num_freq - minimum


def aggregate_datapoints(dirpath, testcases, shifts):
    aggregate = {}
    for dirname, dirnames, filenames in os.walk(dirpath):
        # print path to all filenames.
        for filename in filenames:
            if not any(filename.startswith(testcase) for testcase in testcases.split(',')):
                continue

            try:
                filepath = os.path.join(dirname, filename)
                #print "Reading file [%s]" % (filepath,)
                f = open(filepath, 'r')
                text = f.read()
                data = json.loads(text)
                f.close()
                has_shift = shifts and any(shift in filename for shift in shifts.split(','))
                average = compute_average(data['datapoints'], has_shift)
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
                    aggregate[im][it][ia][ic] = []
                aggregate[im][it][ia][ic].append(average)
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

    if details:
        print "*" * 64
        print "*" * 64
        print "details"
        print "population", population[0], population[1], population[-2], population[-1]
        print "mean", mean
        print "count_passed: %f" % (float(count_passed),)
        print "num_pop %f" % (float(num_population), )

    print "passed: %f" % (float(count_passed) / float(num_population), )

     


def plot_aggregates(aggregates):
    xs = []
    ys = []
    fig = plt.figure(3)
    ax = fig.add_subplot(111)
    lines = []
    names = []
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
    for im in aggregates.keys():
        print 'im', im
        if im != 'probing_sequence_length_search': continue
        for index, it in enumerate(sorted(aggregates[im].keys())):
            ia_ref = None
            for ia in sorted(aggregates[im][it].keys()):
                if 'probing' in ia:
                    ia_ref = ia
                    break
            for ia in sorted(aggregates[im][it].keys()):
                print 'ia', ia
                #if ia != 'probing': continue
                xs = []
                ys = []
                details = True
                for key, value in sorted(aggregates[im][it][ia].items()):
                    if not value:
                        value = [0]
                    value_ref = aggregates[im][it][ia_ref][key]
                    xs.append(key)
                    if ia != ia_ref:
                        print 't-test ', randomized_paired_sample_t_test(value_ref, value, details)
                    ys.append(sum(value) / len(value))
                    if details == False:
                        details = True
     
                print 'plot %s | %s' % (ia, im)
                if 'probing' in ia:
                    style = '-'
                else:
                    style = '--'
                color = colors[index % len(colors)]
                line_current = ax.plot(xs, ys, style, color=color, linewidth=2)
                lines.append(line_current)
                names.append('%s-%s' % (ia, it))

    #plt.plot([1,2,3,4], [1,4,9,16], 'ro')
    #plt.axis([0, 6, 0, 20])
    #plt.show()

    plt.legend(names, loc='upper left')
    plt.show()

    #pprint.pprint(aggregates)

if __name__=="__main__":
    shifts = ""
    if len(sys.argv) == 4:
        shifts = sys.argv[3]
    agg = aggregate_datapoints(sys.argv[1], sys.argv[2], shifts)
    plot_aggregates(agg)
    #pprint.pprint(agg)
