import { ReactNode } from "react";
import style from "./Metric.module.css";

type props = {
  children: ReactNode;
  title: string;
};

export function Metric({ children, title }: props) {
  return (
    <div className={style.container}>
      <p className={style.title}>{title}</p>
      <div className={style.content}>{children}</div>
    </div>
  );
}
