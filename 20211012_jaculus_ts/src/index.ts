import MujMotorEX from "./motor";

function delay(waitMs) {
  const promise = new Promise(function (resolve, reject) {
    setTimeout(resolve, waitMs);
  });

  return promise;
}

function udelej() {
  return delay(2000)
    .then(function () {
      delay(3000);
    })
    .catch(function (ex) {})
    .catch(function (ex) {})
    .finally(function () {});
}

async function udelejLepe() {
  await  delay(2000)
  console.log("za 2s");

  const b = delay(3000).then(function () {
    console.log("za 3s");
  });

  //await Promise.all([a, b]);
  //console.log("oba hotovo");
}

function main() {
  udelejLepe();
  console.log("nejdriv!");

  const m = new MujMotorEX(4);
  m.setPower(5);
}

main();
