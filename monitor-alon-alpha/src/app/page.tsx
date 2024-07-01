"use client";

import styles from "./page.module.css";
import mqtt from "mqtt";
import { useEffect, useState } from "react";
import { Metric } from "./components/Metric";

const TEMPERATURA_TOPIC = "Monitor_Alon_Alpha/RMTZ2000";

export default function Home() {
  const [metrics, setMetrics] = useState({
    temperature: "",
  });

  useEffect(() => {
    const client = mqtt.connect("ws://172.21.68.55:3378");
    client.on("connect", handleConnection);

    function handleConnection() {
      client.subscribe(TEMPERATURA_TOPIC);

      client.on("message", (topic, message) => {
        if (topic === TEMPERATURA_TOPIC) {
          setMetrics({
            ...metrics,
            temperature: message.toString(),
          });
        }
      });
    }
  }, []);

  return <main className={styles.main}>{metrics.temperature}</main>;
}
