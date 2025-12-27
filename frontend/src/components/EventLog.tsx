import type { NetEvent } from '../types';

interface EventLogProps {
  events: NetEvent[];
}

function formatTime(ts: number): string {
  const date = new Date(ts);
  return date.toLocaleTimeString();
}

export function EventLog({ events }: EventLogProps) {
  if (events.length === 0) {
    return (
      <div className="bg-slate-800 rounded-lg p-4 border border-slate-700">
        <h3 className="text-lg font-semibold text-white mb-3">Event Log</h3>
        <p className="text-slate-400 text-sm">No events recorded yet.</p>
      </div>
    );
  }

  // Show newest first
  const sortedEvents = [...events].reverse();

  return (
    <div className="bg-slate-800 rounded-lg p-4 border border-slate-700">
      <h3 className="text-lg font-semibold text-white mb-3">Event Log</h3>
      <div className="space-y-2 max-h-64 overflow-y-auto">
        {sortedEvents.map((event, index) => (
          <div
            key={`${event.ts}-${index}`}
            className="bg-slate-900 rounded p-3 border-l-4 border-red-500"
          >
            <div className="flex justify-between items-start mb-1">
              <span className="text-red-400 font-medium text-sm">
                {event.target_id}
              </span>
              <span className="text-slate-500 text-xs">
                {formatTime(event.ts)}
              </span>
            </div>
            <p className="text-slate-300 text-sm">{event.reason}</p>
            <div className="text-slate-500 text-xs mt-1">
              {Object.entries(event.details).map(([key, value]) => (
                <span key={key} className="mr-3">
                  {key}: {typeof value === 'number' ? value.toFixed(1) : value}
                </span>
              ))}
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
