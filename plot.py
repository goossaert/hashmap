# make the algorithms an argument
# make the metrics an argument
# Re-read the "FelixDB Note" file

import os
import sys
import json

import matplotlib.pyplot as plt
import pprint


def compute_average(datapoints):
    #print datapoints
    num_freq = 0
    sum_metric = 0
    for key, value in datapoints.iteritems():
        occurrence = float(key)
        frequency = float(value)
        num_freq += frequency
        sum_metric += frequency * occurrence
    return sum_metric / num_freq


def aggregate_datapoints(dirpath, testcase):
    aggregate = {}
    for dirname, dirnames, filenames in os.walk(dirpath):
        # print path to all filenames.
        for filename in filenames:
            if filename.startswith(testcase):
                filepath = os.path.join(dirname, filename)
                #print "Reading file [%s]" % (filepath,)
                f = open(filepath, 'r')
                text = f.read()
                data = json.loads(text)
                f.close()
                average = compute_average(data['datapoints'])
                #print "average %f" % (avg)
                ia = data['algorithm']
                im = data['metric']
                ii = data['instance']
                ic = data['cycle']
                if ia not in aggregate:
                    aggregate[ia] = {}
                if im not in aggregate[ia]:
                    aggregate[ia][im] = {}
                if ic not in aggregate[ia][im]:
                    aggregate[ia][im][ic] = []
                aggregate[ia][im][ic].append(average)

    return aggregate 

def plot_aggregates(aggregates):
    xs = []
    ys = []
    for ia in aggregates.keys():
        if ia != 'probing': continue
        for im in aggregates[ia]:
            if im != 'probing_sequence_length_search': continue
            pprint.pprint(aggregates[ia][im])
            for key, value in sorted(aggregates[ia][im].items()):
                if not value:
                    value = [0]
                xs.append(key)
                ys.append(sum(value) / len(value))
    fig = plt.figure(1)
    ax = fig.add_subplot(111)
    line_current = ax.plot(xs, ys, linewidth=1)
    plt.show()

    #plt.plot([1,2,3,4], [1,4,9,16], 'ro')
    #plt.axis([0, 6, 0, 20])
    #plt.show()

if __name__=="__main__":
    agg = aggregate_datapoints(sys.argv[1], sys.argv[2])
    plot_aggregates(agg)
    #pprint.pprint(agg)
