import { useState, useEffect } from 'react';
import type { Target, Thresholds } from '../types';

interface TargetCardProps {
  target: Target;
  thresholds?: Thresholds;
}

const MAX_SAMPLES = 120;
const WARMING_THRESHOLD = 20;

function getStatusColor(value: number, threshold: number): string {
  const ratio = value / threshold;
  if (ratio > 1) return 'text-red-400';
  if (ratio > 0.8) return 'text-yellow-400';
  return 'text-green-400';
}

function MetricRow({ label, value, unit, threshold, isWarming }: {
  label: string;
  value: number;
  unit: string;
  threshold?: number;
  isWarming?: boolean;
}) {
  if (isWarming) {
    return (
      <div className="flex justify-between items-center py-1">
        <span className="text-slate-400 text-sm">{label}</span>
        <span className="font-mono text-slate-500 text-sm italic">
          Warming up...
        </span>
      </div>
    );
  }

  const colorClass = threshold !== undefined
    ? getStatusColor(value, threshold)
    : 'text-slate-300';

  return (
    <div className="flex justify-between items-center py-1">
      <span className="text-slate-400 text-sm">{label}</span>
      <span className={`font-mono font-medium ${colorClass}`}>
        {value.toFixed(1)}{unit}
      </span>
    </div>
  );
}

function formatTimeAgo(ms: number): string {
  const seconds = Math.floor(ms / 1000);
  if (seconds < 1) return 'just now';
  if (seconds < 60) return `${seconds}s ago`;
  const minutes = Math.floor(seconds / 60);
  return `${minutes}m ago`;
}

export function TargetCard({ target, thresholds }: TargetCardProps) {
  const { metrics, samples } = target;
  const isOnline = metrics.current_rtt_ms > 0;
  const sampleCount = samples.length;
  const isWarming = sampleCount < WARMING_THRESHOLD;

  // Get last update time from newest sample
  const lastSample = samples[samples.length - 1];
  const lastUpdateMs = lastSample?.ts ?? 0;

  // Track time since last update
  const [timeSinceUpdate, setTimeSinceUpdate] = useState(0);

  useEffect(() => {
    if (!lastUpdateMs) return;

    const updateTime = () => {
      setTimeSinceUpdate(Date.now() - lastUpdateMs);
    };

    updateTime();
    const interval = setInterval(updateTime, 500);
    return () => clearInterval(interval);
  }, [lastUpdateMs]);

  const isStale = timeSinceUpdate > 5000;

  return (
    <div className="bg-slate-800 rounded-lg p-4 shadow-lg border border-slate-700">
      {/* Header */}
      <div className="flex justify-between items-start mb-1">
        <div>
          <h3 className="text-lg font-semibold text-white">{target.label}</h3>
          <p className="text-slate-400 text-sm">{target.host}:{target.port}</p>
        </div>
        <div className="flex flex-col items-end gap-1">
          <div className={`px-2 py-1 rounded text-xs font-medium ${
            isOnline ? 'bg-green-900 text-green-300' : 'bg-red-900 text-red-300'
          }`}>
            {isOnline ? 'Online' : 'Offline'}
          </div>
          {lastUpdateMs > 0 && (
            <span className={`text-xs ${isStale ? 'text-red-400' : 'text-slate-500'}`}>
              {isStale ? 'Stale' : formatTimeAgo(timeSinceUpdate)}
            </span>
          )}
        </div>
      </div>

      {/* Sample counter */}
      <div className="flex items-center gap-2 mb-3">
        <div className="flex-1 bg-slate-700 rounded-full h-1.5">
          <div
            className="bg-blue-500 h-1.5 rounded-full transition-all duration-300"
            style={{ width: `${(sampleCount / MAX_SAMPLES) * 100}%` }}
          />
        </div>
        <span className="text-slate-500 text-xs font-mono">
          {sampleCount}/{MAX_SAMPLES}
        </span>
      </div>

      {/* Metrics */}
      <div className="space-y-1 border-t border-slate-700 pt-3">
        <MetricRow
          label="Current RTT"
          value={metrics.current_rtt_ms}
          unit="ms"
        />
        <MetricRow
          label="Max RTT"
          value={metrics.max_rtt_ms}
          unit="ms"
        />
        <MetricRow
          label="Packet Loss"
          value={metrics.loss_pct}
          unit="%"
          threshold={thresholds?.loss_pct}
        />
        <MetricRow
          label="Jitter"
          value={metrics.jitter_ms}
          unit="ms"
          threshold={thresholds?.jitter_ms}
          isWarming={isWarming}
        />
        <MetricRow
          label="P50 Latency"
          value={metrics.p50_ms}
          unit="ms"
          isWarming={isWarming}
        />
        <MetricRow
          label="P95 Latency"
          value={metrics.p95_ms}
          unit="ms"
          threshold={thresholds?.p95_ms}
          isWarming={isWarming}
        />
      </div>

      {/* Thresholds footer */}
      {thresholds && (
        <div className="mt-3 pt-2 border-t border-slate-700">
          <p className="text-slate-500 text-xs">
            Thresholds: loss&gt;{thresholds.loss_pct}% · p95&gt;{thresholds.p95_ms}ms · jitter&gt;{thresholds.jitter_ms}ms
          </p>
        </div>
      )}
    </div>
  );
}
