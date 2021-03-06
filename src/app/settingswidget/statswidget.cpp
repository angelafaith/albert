// Copyright (C) 2014-2018 Manuel Schneider

#include "statswidget.h"
#include <QSqlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>

StatsWidget::StatsWidget(QWidget *parent) : QChartView(parent)
{
    _dateTimeAxis = new QDateTimeAxis;
    _dateTimeAxis->setTickCount(8);
    _dateTimeAxis->setFormat("dd.MM.");
    _dateTimeAxis->setLabelsAngle(-45);
    _dateTimeAxis->setTitleText("Date");

    _valueAxis_activations = new QValueAxis;
    _valueAxis_activations->setLabelFormat("%i");
    _valueAxis_activations->setTitleText("Activations per day");
//    valueAxis_activations->setMinorTickCount(1);

    _valueAxis_cumsum = new QValueAxis;
    _valueAxis_cumsum->setLabelFormat("%i");
    _valueAxis_cumsum->setTitleText("Cumulative activations");

    _chart = new QChart();
    _chart->layout()->setContentsMargins(0, 0, 0, 0);
    _chart->setBackgroundRoundness(0);
//    chart->setTitle("Usage");
    _chart->addAxis(_dateTimeAxis, Qt::AlignBottom);
    _chart->addAxis(_valueAxis_activations, Qt::AlignLeft);
    _chart->addAxis(_valueAxis_cumsum, Qt::AlignRight);

    setChart(_chart);
    setRenderHint(QPainter::Antialiasing);
}

void StatsWidget::showEvent(QShowEvent *)
{
    updateChart();
}

void StatsWidget::updateChart()
{
    _chart->removeAllSeries();

    // Add user data

    QLineSeries *activationSeries = new QLineSeries();
    activationSeries->setName("Your daily activations");
    QLineSeries *cumSumSeries = new QLineSeries();
    cumSumSeries->setName("Your cumulative activations");

    for (auto & series : {activationSeries, cumSumSeries}) {
        QPen pen = series->pen();
        pen.setWidth(1);
        pen.setColor("#00cccc");
        series->setPen(pen);
    }
    QPen pen = cumSumSeries->pen();
    pen.setStyle(Qt::DashDotLine);
    cumSumSeries->setPen(pen);

    QSqlQuery q("SELECT q.timestamp, COUNT(*) "
        "FROM activation a JOIN query q ON a.query_id == q.id "
        "GROUP BY date(q.timestamp, 'unixepoch') "
        "HAVING q.timestamp > strftime('%s', 'now', '-30 days');");
    double cumsum = 0;
    while (q.next()) {
        int activationsPerDay = q.value(1).toInt();
        cumsum += activationsPerDay;
        activationSeries->append(q.value(0).toLongLong()*1000, activationsPerDay);
        cumSumSeries->append(q.value(0).toLongLong()*1000, cumsum);
    }
    _chart->addSeries(activationSeries);
    _chart->addSeries(cumSumSeries);

    activationSeries->attachAxis(_dateTimeAxis);
    activationSeries->attachAxis(_valueAxis_activations);

    cumSumSeries->attachAxis(_dateTimeAxis);
    cumSumSeries->attachAxis(_valueAxis_cumsum);

    _valueAxis_activations->setRange(0, _valueAxis_activations->max());
    _valueAxis_cumsum->setRange(0, _valueAxis_cumsum->max());


    // Add global data

    QString addr = "Zffb,!!*\" $## $\"' **!";
    for ( auto &c: addr)
        c.unicode()=c.unicode()+14;
    addr.append("api/activations?days=30");

    static QNetworkAccessManager *manager = new QNetworkAccessManager;
    QNetworkReply* reply = manager->get(QNetworkRequest(QUrl(addr)));

    QObject::connect(reply, &QNetworkReply::finished, [reply, this](){
        if (reply->error() == QNetworkReply::NoError){

            QLineSeries *activationSeries = new QLineSeries();
            activationSeries->setName("Global Ø daily activations");
            QLineSeries *cumSumSeries = new QLineSeries();
            cumSumSeries->setName("Global Ø cumulative activations");

            for (auto & series : {activationSeries, cumSumSeries}) {
                QPen pen = series->pen();
                pen.setWidth(1);
                pen.setColor("#FF7400");
                series->setPen(pen);
            }
            QPen pen = cumSumSeries->pen();
            pen.setStyle(Qt::DashDotLine);
            cumSumSeries->setPen(pen);

            QJsonArray jsonArray = QJsonDocument::fromJson(reply->readAll()).array();
            double cumSum = 0;
            for (QJsonValueRef jsonValue : jsonArray) {
                const QJsonObject &jsonObject = jsonValue.toObject();
                /*
                 * {
                 *   "date": "2018-10-17",
                 *   "reports": 5941,
                 *   "activations": 32238
                 * },
                 */
                double avgActivationsPerDay = jsonObject.value("activations").toDouble() / jsonObject.value("reports").toDouble();
                int64_t msTimestamp = QDateTime::fromString(jsonObject.value("date").toString(), "yyyy-MM-dd").toMSecsSinceEpoch();
                cumSum += avgActivationsPerDay;
                activationSeries->append(msTimestamp, avgActivationsPerDay);
                cumSumSeries->append(msTimestamp, cumSum);
            }

            _chart->addSeries(activationSeries);
            _chart->addSeries(cumSumSeries);

            activationSeries->attachAxis(_dateTimeAxis);
            activationSeries->attachAxis(_valueAxis_activations);

            cumSumSeries->attachAxis(_dateTimeAxis);
            cumSumSeries->attachAxis(_valueAxis_cumsum);

            _valueAxis_activations->setRange(0, _valueAxis_activations->max());
            _valueAxis_cumsum->setRange(0, _valueAxis_cumsum->max());
        }
        reply->deleteLater();
    });
}
