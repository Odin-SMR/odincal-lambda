= Odin Calibration (odincal)

In this repository you can find tools for processing Odin L0 to L1b.

The full calibration processing can be run in a sandbox locally using docker
and `docker-compose`.

For production the odincal-image is run on _malachite_ (the production
environment) with a `docker-compose.yml` specially designed for that
environment.

To set up a local test-system:

  docker-compose build
  docker-compose up

Add some files to the system

  tar xf data/test_data.tar.bz -C data/level0/

Import the data (~2 min)

  docker exec odincal_odincal_1 process_level0_or_calibrate

Run the same command to perform the calibration (~1 hour)

  docker exec odincal_odincal_1 process_level0_or_calibrate

Database access is possible with access through docker

  docker exec -ti odincal_postgres_1 psql -U odinop odin

or using a database client directly, the password is "secret"

  psql -h localhost -U odinop  odin


== Developers

There are a limited set of unittests that can be run using tox.
