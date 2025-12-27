import type { Target, Thresholds } from '../types';

interface HealthGradeProps {
  targets: Target[];
  thresholds?: Thresholds;
}

type Grade = 'A' | 'B' | 'C' | 'D' | 'F';

interface GradeInfo {
  grade: Grade;
  color: string;
  bgColor: string;
  message: string;
}

function calculateGrade(targets: Target[], thresholds?: Thresholds): GradeInfo {
  if (targets.length === 0) {
    return {
      grade: 'A',
      color: 'text-slate-400',
      bgColor: 'bg-slate-700',
      message: 'No targets configured',
    };
  }

  // Check for offline targets or severe packet loss
  const offlineCount = targets.filter(t => t.metrics.current_rtt_ms <= 0).length;
  const severePacketLoss = targets.some(t => t.metrics.loss_pct > 10);

  if (offlineCount > 0 || severePacketLoss) {
    const reasons: string[] = [];
    if (offlineCount > 0) {
      reasons.push(`${offlineCount} target${offlineCount > 1 ? 's' : ''} offline`);
    }
    if (severePacketLoss) {
      reasons.push('>10% packet loss');
    }
    return {
      grade: 'F',
      color: 'text-red-400',
      bgColor: 'bg-red-900/50',
      message: reasons.join(' · '),
    };
  }

  if (!thresholds) {
    // No thresholds configured, assume healthy if all online
    return {
      grade: 'A',
      color: 'text-green-400',
      bgColor: 'bg-green-900/50',
      message: 'All targets online',
    };
  }

  // Count red and yellow metrics
  let redCount = 0;
  let yellowCount = 0;

  for (const target of targets) {
    const { metrics } = target;

    // Check each metric against thresholds
    const checks = [
      { value: metrics.loss_pct, threshold: thresholds.loss_pct },
      { value: metrics.p95_ms, threshold: thresholds.p95_ms },
      { value: metrics.jitter_ms, threshold: thresholds.jitter_ms },
    ];

    for (const { value, threshold } of checks) {
      const ratio = value / threshold;
      if (ratio > 1) {
        redCount++;
      } else if (ratio > 0.8) {
        yellowCount++;
      }
    }
  }

  // Calculate overall packet loss
  const totalLoss = targets.reduce((sum, t) => sum + t.metrics.loss_pct, 0);
  const avgLoss = totalLoss / targets.length;

  if (redCount >= 3) {
    return {
      grade: 'D',
      color: 'text-orange-400',
      bgColor: 'bg-orange-900/50',
      message: `${redCount} metrics above threshold`,
    };
  }

  if (redCount >= 1) {
    return {
      grade: 'C',
      color: 'text-yellow-400',
      bgColor: 'bg-yellow-900/50',
      message: `${redCount} metric${redCount > 1 ? 's' : ''} above threshold`,
    };
  }

  if (yellowCount >= 1) {
    return {
      grade: 'B',
      color: 'text-blue-400',
      bgColor: 'bg-blue-900/50',
      message: `${yellowCount} metric${yellowCount > 1 ? 's' : ''} approaching threshold`,
    };
  }

  return {
    grade: 'A',
    color: 'text-green-400',
    bgColor: 'bg-green-900/50',
    message: `All targets healthy · ${avgLoss.toFixed(1)}% packet loss`,
  };
}

export function HealthGrade({ targets, thresholds }: HealthGradeProps) {
  const { grade, color, bgColor, message } = calculateGrade(targets, thresholds);

  return (
    <div className={`${bgColor} rounded-lg p-4 mb-6 border border-slate-700`}>
      <div className="flex items-center gap-4">
        <div className={`text-5xl font-bold ${color}`}>
          {grade}
        </div>
        <div>
          <h2 className="text-white font-semibold text-lg">Network Health</h2>
          <p className="text-slate-400 text-sm">{message}</p>
        </div>
      </div>
    </div>
  );
}
