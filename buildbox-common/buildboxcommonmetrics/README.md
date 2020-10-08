# buildboxcommonmetrics
`buildboxcommonmetrics` is a set of utilities to gather and publish timing metrics provided by `buildbox-common`. It follows the format of the [StatsD](https://github.com/statsd/statsd) protocol:

```
<metric_name>:<value>|<metric_type>
```

For example: `"digest_time:2|ms"` describes a `Timing` metric named `digest_time` whose value is `2`.<sup>+</sup>

Although currently in the same library, `buildboxcommonmetrics` is intended to be stand-alone from `buildboxcommon` and as such does not depend on it.

# Supported metric types
`buildbox-common` supports four types of metrics
* `CountingMetricValue`, which implements [Count](https://github.com/statsd/statsd/blob/master/docs/metric_types.md#counting)
* `GaugeMetricValue`, which implements [Gauge](https://github.com/statsd/statsd/blob/master/docs/metric_types.md#gauges)
* `DurationMetricValue`<sup>*</sup>, which implements [Timing](https://github.com/statsd/statsd/blob/master/docs/metric_types.md#timing)
* `DistributionMetricValue`, which implements [Distribution](https://docs.datadoghq.com/metrics/distributions/)

------------------------------
<sub>+ While the name of Timing metric types `ms` looks like it would stand for `milliseconds`, it doesn't
necessarily need to be a number of milliseconds, it could be any unit of time. This library however only
uses milliseconds for it's timing metrics.</sub>

<sub>* An additional MetricType, called `TotalDurationMetricValue`, which sums up all the timings when used multiple
times. This is useful for timing things that are recursive in nature.</sub>

# Initializing
First things first, a publisher needs to be initialized. A publisher determines what format the metrics are emitted, and currently the only supported
publisher is `StatsDPublisher`. Publishers can be constructed to publish to different locations, including:
* stderr
* a file
* a udp address

Only a single publisher should be initialized per process, ideally in main, and must be initialized before attempting to publish metrics.
The publisher can be constructed and configured with the help of the `MetricsConfigUtil`

## `MetricsConfigUtil`
The `MetricsConfigUtil` class provides a convenient way for executables to configure a metric publisher from command-line options. It currently provides two options:
* `--metrics-mode` to configure where to publish metrics, and supports the following formats:
  * `Stderr`
  * `file:///absolute/path/to/file` or `file:://relative/path/file`
  * `udp://host:port` or `udp://host` with a default port of `8125`
* `--metrics-publish-interval` which is used by [ScopedPeriodicPublisherDaemon](#scopedperiodicpublisherdaemon) covered below. Defaults to `30` seconds

```c++
MetricsConfigType metricConfig;
while(arguments) {
  //parse metric options into configuration
  if(MetricsConfigUtil::isMetricsOption(arg)) {
    MetricsConfigUtil::metricsParser(arg, value, &metricConfig)
    continue
  }
  //app specific parsing
  ...
}
// construct a StatsDPublisher from populated configuration
StatsDPublisherType publisher = StatsdPublisherCreator::createStatsdPublisher(config);
```

# Publishing
There are two different wrappers around publishers which determine when collected metrics are published; `PublisherGuard` and `ScopedPeriodicPublisherDaemon`

## `PublisherGuard`

The `PublisherGuard` class wraps a publisher instance and, when it goes out of scope, it will invoke the publisher's `publish()` method. This is intended for short lived processes, where periodicly publishing metrics is unnecessary.


```c++
{
  PublisherGuard<StatsDPublisherType>
  statsDPublisherGuard(config.enable(), publisher);
  // Code that executes and leads to gathered metrics
  ...
}
// Metrics published as PublisherGuard goes out of scope
```

## `ScopedPeriodicPublisherDaemon`

The `ScopedPeriodicPublisherDaemon` class is another wrapper for a publisher instance which publishes any collected
metrics at the configured interval. This is intended for long lived processes, which continuously gather metrics though their lifetimes.

```c++
ScopedPeriodicPublisherDaemon<StatsDPublisherType>
statsDPublisherGuard(config.enable(), config.interval(), publisher);

doStuffContinuously(); // Metrics published every interval seconds even while doing other work
```
# Gathering
One a publisher has been initialized, metrics can start being recorded.

## DurationMetricTimer
Timing metrics for the `DurationMetricTimer` can be performed using the `MetricGuard` class, which will
start/stop a timer when being constructed/destructed. It then calculates the duration in milliseconds,
and marks the metric as being ready for publishing. If a metric of the same name is constructed twice,
two distinct measurements will be recorded and published.

For example:
```c++
{ // Timed block start
  MetricGuard<DurationMetricTimer> mt("action_cache_query");
  ...
  result = query_action_cache();
  ...
} // Timed block end, initial duration recorded and ready to be published
...
{
  MetricGuard<DurationMetricTimer> mt("action_cache_query")
  ...
  result2 = query_action_cache();
  ...
} // Timed block end, second duration d2 recorded and ready to be published
```
In this case, the time it takes to execute that block will be counted as `action_cache_query`

## TotalDurationMetricTimer
The behavior of `TotalDurationMetricTimer` is identical to `DurationMetricTimer` unless the same metric is recorded twice. In that
case, the durations will be added together, producing a total duration.
```c++
{ // Timed block start
  MetricGuard<TotalDurationMetricTimer> mt("action_cache_query");
  ...
  result = query_action_cache();
  ...
} // Timed block end, initial duration d1 recorded and ready to be published
...
{
  MetricGuard<TotalDurationMetricTimer> mt("action_cache_query")
  ...
  result2 = query_action_cache();
  ...
} // Timed block end, duration updated to d1+d2 and ready to be published
```

## CountingMetricValue
Counter metrics can be used in two common ways, either scoping the metric using `ScopedMetric/MetricGuard`
or constructing/incrementing the metric directly and invoking `CountingMetricUtil::recordCounterMetric`
to mark it ready for publishing. If `CountingMetric` is created multiple times with the same name,
the values are added together.

```c++
int i = 0;
while(i < 10) {
  std::cout << "On iteration: " << i << std::endl;
}
// Increases the counter value "number_of_iterations" by 10 and marks it ready for publishing
CountingMetricUtil::recordCounterMetric("number_of_iterations", i);
```

Additionally, a `CountingMetric` can be constructed and manipulated manually
```c++
{
  CountingMetric m("number_of_iterations");
  while(i < 10) {
    // increments m by 1 at construction
    ScopedMetric<CountingMetric>(&m);
    std::cout << "On iteration: " << i << std::endl;
    // Marks the metric ready for publishing at destruction
  }
  int j = 0;
  while (j < 100) {
    ...
  }
  // Use operator+= to increase the count by a specific value
  m += j;
  // explicitly publish it
  CountingMetricUtil::recordCounterMetric(m);
}
{
  CountingMetric m2("number_of_iterations");
  m2 += 5;
  // This will be added to the existing metric of the same name
  CountingMetricUtil::recordCounterMetric(m2);
}
```
## GaugeMetricValue
Gauge metrics are published using methods provided by `GaugeMetricUtil`. `setGauge` will set a gauge to
a specific value, and `adjustGauge` will change a gauge's value by some positive or negative delta.

```c++
// Set the gauge to a specified value
int initial_status = getStatus();
GaugeMetricUtil::setGauge("gauge0", initial_status);
...
// Adjust a gauge by some delta
while (!done) {
    progress_made = work();
    GaugeMetricUtil::adjustGauge("gauge0", progress_made);
}
```

## DistributionMetricValue
Distribution metrics allow to publish discrete values that will be aggregated on the server side to determine the statistical distribution of that set of values.

```c++
for (const &digest: digests) {
    DistributionMetricUtil::recordDistributionMetric("blob-sizes", digest.size_bytes());
}
```

Because this metric type is not aggregated, in the example above the number of values published will be equal to the number of entries in `digests`.

# Testing Metric Collection
`buildboxcommonmetrics` comes with several testing methods in `buildboxcommonmetrics_testingutils` to verify metrics are being published as expected in a unit test.
```c++
// Validate a timing metric was collected
void doTimedStuff()
{
  MetricGuard<DurationMetricTimer> mt("foo_timer");
  CountingMetric m1("foo_count1");
  m1++
  doStuff();
  CountingMetric m1("foo_count2");
  m2++
}

TEST(FooTest, FooTiming)
{
  doTimedStuff();
  // Tests that the metric was collected
  ASSERT_TRUE(collectedByName<DurationMetricValue>("foo_timer"));
  // After a call to collectedByName, the metrics for that ValueType are cleared
  ASSERT_FALSE(collectedByName<DurationMetricValue>("foo_timer"));

  // Validate that multiple metrics of the same type were collected
  ASSERT_TRUE(allCollectedByName<CountingMetricValue>({"foo_count1", "foo_count2"}))
}
```

# Example
```c++
//pseudo-code example
int main(...){
  MetricsConfigType metricConfig;
  while(arguments) {
    //parse metric options into configuration
    if(MetricsConfigUtil::isMetricsOption(arg)) {
      MetricsConfigUtil::metricsParser(arg, value, &metricConfig)
      continue
    }
    //app specific parsing
    ...
  }
  StatsDPublisherType publisher = StatsdPublisherCreator::createStatsdPublisher(config);

  PublisherGuard<StatsDPublisherType>
  statsDPublisherGuard(config.enable(), publisher);
  {
    CountingMetric success("success_count");
    CountingMetric failure("failure_count");
    MetricGuard<DurationMetricTimer> mt("loop_duration");

    for(int i = 0; i < 1000; i++) {
      int rc = business_logic();
      if(rc == 0) {
        success++;
      } else {
        failure++;
      }
    }

  }
  return 0;
}
```
