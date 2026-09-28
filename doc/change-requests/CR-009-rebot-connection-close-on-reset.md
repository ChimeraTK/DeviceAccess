# CR-009: Rebot connection close must not throw on a reset connection

Synopsis: The spurious failure of `testRebotBackendCreation` (fatal
`boost::system::system_error` "shutdown: Transport endpoint is not connected",
`ENOTCONN`) comes from `Rebot::Connection::close()` throwing when the peer
already closed or reset the transport. Make `close()` non-throwing so a lost
connection surfaces as the documented `ChimeraTK::runtime_error`, and add
regression tests.

Status: DONE

## Requirements

Aspect: closing a Rebot connection on a lost transport.

- Closing a Rebot connection (`RebotBackend::closeImpl()`, connection timeout,
device close or destruction) must never throw, even when the peer already
closed or reset the transport before the close. Currently such a close
throws an unhandled `boost::system::system_error` (`ENOTCONN`), which
aborts the calling process/test.
- When the transport is lost, the triggering read or write must throw the
documented `ChimeraTK::runtime_error` ("Rebot connection timed out"), never a
`boost::system::system_error`.

## Specifications

Aspect: non-throwing socket teardown in `Rebot::Connection`.

- Affected component: `backends/Rebot/src/Connection.cc`,
`Connection::close()`.
- A socket that received a reset from the peer still reports `is_open() ==
true`, so the existing `is_open()` guard cannot detect the lost transport;
the throwing `shutdown()` overload then fails with `ENOTCONN`. After an
`error_code` close the socket may still report `is_open() == true`; this is
harmless because close errors are discarded and the disconnect timer is not
re-armed after a close, so no close/retry loop can form.
- Why the failure is fatal: the throw originates inside the completion handler
of an async operation, escapes `ioContext_.run()`, and the
`catch(boost::exception&)` in `Connection::open()` does not intercept
`boost::system::system_error` (it is not derived from `boost::exception`),
so the abort propagates to the caller.
- Replace the throwing `cancel()`, `shutdown()` and `close()` calls in
`Connection::close()` with the `boost::system::error_code` overloads and
discard the error code. Benign errors such as `ENOTCONN` and `EBADF` are
then ignored.
- The change covers all three close call sites at once: `closeImpl()`, the
disconnect timer handler and `Connection::disconnectionTimerCancel()`.
- `disconnectionTimerCancel()` stays unchanged: with `close()` no longer
throwing, real failures reach its intended
`throw ChimeraTK::runtime_error("Rebot connection timed out")`. Today the
throwing `close()` would mask this exception at `Connection.cc:82`; the
change removes that masking.
- The heartbeat loop needs no further catch. This claim is scoped to the
`Connection` class: its operations then throw only `ChimeraTK::runtime_error`,
which `RebotBackend::heartbeatLoop` already catches; `system_error` can no
longer escape `Connection`. The protocol layer (`RebotProtocol0.cc`) also
throws `ChimeraTK::logic_error` at read/write entry, but the heartbeat only
performs `Connection` operations (in protocol 0 `sendHeartbeat()` is a
no-op), so this does not affect the heartbeat path.
- Concurrency note: `closeImpl()` (user thread) and the timer-driven close
(io thread) cannot run concurrently; both close paths are serialised by the
`_closeMutex` try-lock in `Connection::close()`
(`backends/Rebot/src/Connection.cc:51-55`). The `_threadInformerMutex` at
`RebotBackend` level serialises `open()`, `read()`, `write()` and
`closeImpl()` only; the timer-driven close (the `disconnectionTimerStart`
handler and `disconnectionTimerCancel()`) calls `close()` directly on the
io thread without taking it. The `_closeMutex` guard for the close paths
is unchanged.

### Alternatives considered

- Catch `boost::system::system_error` at every call site of `Connection::close()`:
rejected, duplicative and easy to miss a future call site; the `error_code`
overloads are the idiomatic asio solution.

## Test plan

- Regression (in `tests/executables_src/testRebotConnectionTimeouts.cpp` or
`testRebotHeartbeatCount.cpp` style, own in-process `RebotDummyServer`):
with one device occupying the mock server's single session, a second device
(a different URI on the same server port) connects. The server accepts and
then closes the second connection, which the client sees as a connection
reset. Depending on timing this error surfaces at `open()` or at the first
IO, so assert BOTH: the device's open and its first read/write raise only
`ChimeraTK::runtime_error`, and closing it afterwards does not throw.
- Regression: close a device whose connection the server reset mid-operation;
assert the close does not throw. This needs a new reset-injection helper in
the mock server: the session socket is owned by the server io thread, so the
test cannot close it from the test thread. Add e.g. a method on
`RebotDummyServer`/session that posts the socket close onto the server io
loop.
- Existing tests keep passing: `testRebotConnectionTimeouts` (timeout yields
`ChimeraTK::runtime_error`), `testRebotBackend`, `testRebotHeartbeatCount`.
`testRebotBackendCreation` lives in `tests/unitTestsNotUnderCtest` and
requires a real device, so it is not verifiable in CI; the regression
evidence rests on the new in-process tests above. A new test file in
`tests/executables_src` is registered automatically via
`aux_source_directory` in `tests/CMakeLists.txt`.
