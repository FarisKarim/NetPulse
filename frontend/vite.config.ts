import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    allowedHosts: ['.gitpod.dev'],
    proxy: {
      '/api': {
        target: 'http://localhost:7331',
        changeOrigin: true,
      },
      '/ws': {
        target: 'ws://localhost:7331',
        ws: true,
      },
    },
  },
})
