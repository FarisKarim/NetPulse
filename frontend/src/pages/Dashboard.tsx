import { useMetricsStore } from '../stores/metricsStore';
import { TargetCard } from '../components/TargetCard';
import { RttChart } from '../components/RttChart';
import { EventLog } from '../components/EventLog';
import { HealthGrade } from '../components/HealthGrade';

export function Dashboard() {
  const { targets, config, events } = useMetricsStore();

  const targetList = Array.from(targets.values());

  if (targetList.length === 0) {
    return (
      <div className="p-6">
        <h1 className="text-2xl font-bold text-white mb-6">Dashboard</h1>
        <div className="bg-slate-800 rounded-lg p-8 text-center border border-slate-700">
          <p className="text-slate-400">Waiting for data...</p>
          <p className="text-slate-500 text-sm mt-2">
            Make sure the NetPulse daemon is running on port 7331
          </p>
        </div>
      </div>
    );
  }

  return (
    <div className="p-6">
      <h1 className="text-2xl font-bold text-white mb-6">Dashboard</h1>

      {/* Overall Health Grade */}
      <HealthGrade targets={targetList} thresholds={config?.thresholds} />

      {/* Target Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4 mb-6">
        {targetList.map((target) => (
          <TargetCard
            key={target.id}
            target={target}
            thresholds={config?.thresholds}
          />
        ))}
      </div>

      {/* RTT Charts */}
      <div className="space-y-4 mb-6">
        {targetList.map((target) => (
          <RttChart
            key={target.id}
            samples={target.samples}
            label={`${target.label} RTT`}
          />
        ))}
      </div>

      {/* Event Log */}
      <EventLog events={events} />
    </div>
  );
}
