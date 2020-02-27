`buildbox-common` provides utilities to gather and publish timing metrics in a format that follows the [StatsD](https://github.com/statsd/statsd) protocol:

```
<metric_name>:<value>|<unit>
```

For example: `"<digest_time>:2|ms"`.


# Gathering
First, a publisher needs to be initialized. Different kinds of publishers allow to export the metrics to different destinations.

The `PublisherGuard` class wraps a publisher instance and, when it goes out of scope, it will invoke the publisher's `publish()` method.


```c++
// Metrics can be disabled at runtime
const bool metrics_enabled = true;
// Writing to a file:
const StatsDPublisherType publisher_type(StatsDPublisherOptions::PublishMethod::File,
               "/tmp/stats.out", 0);

{
PublisherGuard<StatsDPublisherType> statsDPublisherGuard(metrics_enabled, publisher_type);
...
}
// Metrics published
```


Once the publisher is initialized, timing measurements can be performed by using the `MetricGuard` class and defining scopes.
For example:
```c++
{ // Timed block start
  MetricGuard<DurationMetricTimer> mt("action_cache_query", metrics_enabled);
  ...
  result = query_action_cache();
  ...
} // Timed block end
```

In this case, the time it takes to execute that block will be counted as `action_cache_query`.

# Publishing
Currently, metrics can be published to:

* `StdErr`,
* `File`, or
* `UDP` endpoint.
