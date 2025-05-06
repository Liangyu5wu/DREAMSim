#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <deadtime>"
    echo "Example: $0 10.0"
    exit 1
fi

DEADTIME=$1

root -l <<EOF
.L PlotDeadtimeComparison.C
PlotDeadtimeComparison(20, $DEADTIME, "../deadtimedata/")
PlotDeadtimeComparison(25, $DEADTIME, "../deadtimedata/")
PlotDeadtimeComparison(40, $DEADTIME, "../deadtimedata/")
PlotDeadtimeComparison(50, $DEADTIME, "../deadtimedata/")
PlotDeadtimeComparison(80, $DEADTIME, "../deadtimedata/")
PlotDeadtimeComparison(100, $DEADTIME, "../deadtimedata/")
.q
EOF

python combine_images.py --deadtime $DEADTIME

echo "Processing completed with deadtime = $DEADTIME"
