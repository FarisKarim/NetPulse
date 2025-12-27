import { create } from 'zustand';
import type { Target, Config, NetEvent, Sample, Metrics } from '../types';

interface MetricsState {
  targets: Map<string, Target>;
  config: Config | null;
  events: NetEvent[];
  connected: boolean;

  // Actions
  setSnapshot: (targets: Target[], config: Config) => void;
  addSample: (targetId: string, sample: Sample) => void;
  updateMetrics: (targetId: string, metrics: Metrics) => void;
  addEvent: (event: NetEvent) => void;
  updateConfig: (config: Config) => void;
  setConnected: (connected: boolean) => void;
}

const MAX_SAMPLES = 120;
const MAX_EVENTS = 100;

export const useMetricsStore = create<MetricsState>((set) => ({
  targets: new Map(),
  config: null,
  events: [],
  connected: false,

  setSnapshot: (targets, config) => set(() => {
    const targetMap = new Map<string, Target>();
    for (const target of targets) {
      targetMap.set(target.id, target);
    }
    return { targets: targetMap, config };
  }),

  addSample: (targetId, sample) => set((state) => {
    const target = state.targets.get(targetId);
    if (!target) return state;

    const newSamples = [...target.samples, sample];
    // Keep only last MAX_SAMPLES
    if (newSamples.length > MAX_SAMPLES) {
      newSamples.shift();
    }

    const newTargets = new Map(state.targets);
    newTargets.set(targetId, { ...target, samples: newSamples });
    return { targets: newTargets };
  }),

  updateMetrics: (targetId, metrics) => set((state) => {
    const target = state.targets.get(targetId);
    if (!target) return state;

    const newTargets = new Map(state.targets);
    newTargets.set(targetId, { ...target, metrics });
    return { targets: newTargets };
  }),

  addEvent: (event) => set((state) => {
    const newEvents = [...state.events, event];
    if (newEvents.length > MAX_EVENTS) {
      newEvents.shift();
    }
    return { events: newEvents };
  }),

  updateConfig: (config) => set({ config }),

  setConnected: (connected) => set({ connected }),
}));
