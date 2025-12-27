import { useEffect, useRef } from 'react';
import uPlot from 'uplot';
import 'uplot/dist/uPlot.min.css';
import type { Sample } from '../types';

interface RttChartProps {
  samples: Sample[];
  label: string;
}

export function RttChart({ samples, label }: RttChartProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const chartRef = useRef<uPlot | null>(null);

  useEffect(() => {
    if (!containerRef.current) return;

    // Prepare data for uPlot
    // x-axis: timestamps (in seconds for uPlot)
    // y-axis 1: RTT values (null for failures)
    // y-axis 2: Failures as dots at a fixed position
    const times: number[] = [];
    const rtts: (number | null)[] = [];
    const failures: (number | null)[] = [];

    // First pass: collect RTTs to find max for failure marker position
    let maxRtt = 10;
    for (const sample of samples) {
      if (sample.success && sample.rtt_ms > maxRtt) {
        maxRtt = sample.rtt_ms;
      }
    }
    const failureY = maxRtt * 1.05; // Position failure dots near top

    for (const sample of samples) {
      times.push(sample.ts / 1000); // Convert ms to seconds
      rtts.push(sample.success ? sample.rtt_ms : null);
      failures.push(sample.success ? null : failureY);
    }

    const data: uPlot.AlignedData = [times, rtts, failures];

    const opts: uPlot.Options = {
      width: containerRef.current.clientWidth,
      height: 200,
      title: label,
      series: [
        {},
        {
          label: 'RTT',
          stroke: '#3b82f6',
          width: 2,
          fill: 'rgba(59, 130, 246, 0.1)',
          points: {
            show: false,
          },
        },
        {
          label: 'Failed',
          stroke: '#ef4444',
          width: 0,
          points: {
            show: true,
            size: 8,
            fill: '#ef4444',
            stroke: '#ef4444',
          },
          paths: () => null, // No line connecting points
        },
      ],
      axes: [
        {
          stroke: '#64748b',
          grid: { stroke: '#334155' },
        },
        {
          stroke: '#64748b',
          grid: { stroke: '#334155' },
          size: 60,  // Fixed width for Y-axis to prevent label cutoff
          values: (_, ticks) => ticks.map(v => v.toFixed(0) + 'ms'),
        },
      ],
      scales: {
        x: {
          time: true,
        },
        y: {
          auto: true,
          range: (_u, _min, max) => [0, Math.max(max * 1.1, 10)],
        },
      },
      cursor: {
        show: true,
        points: {
          show: true,
          size: 8,
          stroke: '#3b82f6',
          fill: '#1e293b',
        },
      },
      legend: {
        show: false,
      },
    };

    if (chartRef.current) {
      // Update existing chart
      chartRef.current.setData(data);
    } else {
      // Create new chart
      chartRef.current = new uPlot(opts, data, containerRef.current);
    }

    // Handle resize
    const handleResize = () => {
      if (chartRef.current && containerRef.current) {
        chartRef.current.setSize({
          width: containerRef.current.clientWidth,
          height: 200,
        });
      }
    };

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
    };
  }, [samples, label]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (chartRef.current) {
        chartRef.current.destroy();
        chartRef.current = null;
      }
    };
  }, []);

  return (
    <div className="bg-slate-800 rounded-lg p-4 border border-slate-700">
      <div ref={containerRef} />
    </div>
  );
}
