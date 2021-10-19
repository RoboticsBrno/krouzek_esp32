"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.prepareMotor = exports.MAX_POWER = void 0;
exports.MAX_POWER = 100;
function prepareMotor(id) { }
exports.prepareMotor = prepareMotor;
var Motor = /** @class */ (function () {
    function Motor(id) {
        this.power = 0;
        this.id = id;
    }
    Motor.prototype.setPower = function (p) {
        console.log("Motor " + this.id + " power set to " + p);
        this.power = p;
    };
    return Motor;
}());
exports.default = Motor;
//# sourceMappingURL=motor.js.map