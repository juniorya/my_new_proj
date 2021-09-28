var chart;

function requestData(src) {
    $.ajax({
        url: src,
        type: "GET",
        dataType: "json",
        data : { some : "params"},
        success: function(data) {
            chart.series[0].setData(data.data);
/*
            chart.addSeries({
              name: "Signal",
              data: data.data
            });
*/
        },
        cache: false
    });
}

$(document).ready(function() {
    chart = new Highcharts.Chart({
        chart: {
            renderTo: 'container',
            type: 'line',
            marginRight: 130,
            marginBottom: 25,
            zoomType: 'x',
            panning: true,
            panKey: 'shift'
        },
        title: {
            text: 'Signal',
            x: -20 //center
        },
        xAxis: {
/*            categories: ['1', '2', '3', '4', '5', '6',
                '7', '8', '9', '10', '11', '12'], */
            min: 0,
            max: 10,
            scrollbar: {
                enabled: true
            }
        },
        yAxis: {
            title: {
                text: 'Digital Signal'
            },
            plotLines: [{
                value: 0,
                width: 1,
                color: '#808080'
            }]
        },
        tooltip: {
            formatter: function() {
                    return '<b>'+ this.series.name +'</b><br/>'+
                    this.x +': '+ this.y;
            }
        },
        legend: {
            layout: 'vertical',
            align: 'right',
            verticalAlign: 'top',
            x: -10,
            y: 100,
            borderWidth: 0
        },
        seriesxxx: [{
            name: 'Tokyo',
            data: [[0,0],[1,0],[1,1],[2,1],[2,0],[3,0],[3,1],[4,1],[4,0],[5,0],[5,1],[6,1]]
        }],
        series: [{
            name: 'Signal',
            data: []
        }],
        seriesyyy: [{
            name: 'Signal',
            data: [[0,0],[1.033,0],[1.033,1],[4,1],[4,0],[5,0],[5,1],[6,1],[40.05,1],[40.05,0],[41,0],[41,1]]
        }]
    });

    $(".js-proto-upload").on("click", function(e) {
        e.preventDefault();

        var fd = new FormData();
        var files = $('#file_proto')[0].files;

        // Check file selected or not
        if(files.length == 0) {
            alert("Файл не выбран");
            return 0;
        }

        fd.append('file_proto', files[0]);

        var xhr = $.post({
            url: "/loader",
            data: fd,
            processData: false,
            contentType: false
        });
        xhr.done(function (data) {
            if (data.success) {
                //alert("SUCCESS");
                $(".js-proto-upload-result").addClass("open");
            } else {
                alert("ERROR");
            }
        });
        xhr.fail(function(xhr, status, error) {
            var resp = JSON.parse(xhr.responseText);
            alert("FAIL");
        });
        return false;
    });

    $(".js-set-chart-uploaded").on("click", function(e) {
        e.preventDefault();

        $(".js-proto-upload-result").removeClass("open");

        requestData('/api/loaded/');
        return false;
    });

    requestData('/static/chart.json');
});
