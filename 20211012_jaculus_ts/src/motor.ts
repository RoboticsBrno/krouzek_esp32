export const MAX_POWER = 100;

export type DenVTydnu = 0 | 2 | 3;

export function prepareMotor(id: number) {}

export default class Motor {
  power: number = 0;
  private readonly id: number

  constructor(id: number) {
    this.id = id
  }

  setPower(p: number): void {
    console.log(`Motor ${this.id} power set to ${p}`);
    this.power = p;
  }
}
