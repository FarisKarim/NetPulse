import { useState, useRef, useEffect } from 'react';

const metrics = [
  {
    name: 'Current RTT',
    description: 'Response time of the most recent successful probe',
  },
  {
    name: 'Max RTT',
    description: 'Highest response time in the last 60 seconds — catches spikes that percentiles filter out',
  },
  {
    name: 'Packet Loss',
    description: 'Percentage of probes that timed out or failed in the last 60 seconds',
  },
  {
    name: 'Jitter',
    description: 'Average variation between consecutive RTT measurements — lower means more stable',
  },
  {
    name: 'P50 Latency',
    description: 'Median latency — 50% of probes are faster than this value',
  },
  {
    name: 'P95 Latency',
    description: '95th percentile — only 5% of probes are slower, shows worst-case performance',
  },
];

export function MetricsInfo() {
  const [open, setOpen] = useState(false);
  const dropdownRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    function handleClickOutside(event: MouseEvent) {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
        setOpen(false);
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  return (
    <div className="relative" ref={dropdownRef}>
      <button
        onClick={() => setOpen(!open)}
        className={`flex items-center gap-1.5 px-3 py-2 rounded-md text-sm font-medium transition-colors ${
          open
            ? 'bg-slate-700 text-white'
            : 'text-slate-300 hover:bg-slate-700 hover:text-white'
        }`}
      >
        <svg className="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        Metrics
        <svg
          className={`w-4 h-4 transition-transform ${open ? 'rotate-180' : ''}`}
          fill="none"
          viewBox="0 0 24 24"
          stroke="currentColor"
        >
          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
        </svg>
      </button>

      {open && (
        <div className="absolute top-full left-0 mt-2 w-80 bg-slate-800 rounded-lg border border-slate-700 shadow-xl z-50">
          <div className="px-4 py-3 border-b border-slate-700">
            <h3 className="text-white font-medium">Understanding Your Metrics</h3>
          </div>
          <div className="p-3 space-y-2 max-h-80 overflow-y-auto">
            {metrics.map((metric) => (
              <div
                key={metric.name}
                className="p-2.5 bg-slate-900/50 rounded-lg"
              >
                <h4 className="text-white font-medium text-sm">{metric.name}</h4>
                <p className="text-slate-400 text-xs leading-relaxed mt-0.5">
                  {metric.description}
                </p>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}
