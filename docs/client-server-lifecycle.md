<!-- SPDX-License-Identifier: MPL-2.0 -->

# CLIENT/SERVER lifecycle contract

CLIENT flow: create runtime context, construct complete CLIENT configuration, create and bind a provider, create/connect a transport, attach with explicit ownership, drive handshake/wait, perform I/O, inspect peer, drive shutdown, release.

SERVER flow: application creates listener, bind/listen/accepts, constructs complete SERVER configuration, creates and binds a provider connection, wraps only the accepted connected transport, attaches ownership, drives the same handshake/wait/I/O/shutdown calls, then releases. PST never owns the listener, accept loop, admission, backlog or scheduling.

Selection is complete before transport binding. Ineligible candidates may be skipped only during selection. Once selected, provider identity is immutable and every protocol/authentication/policy failure is terminal for that connection.

The runtime may be logically shared by connections, but API 2.0 does not guarantee concurrent access from multiple threads. Each connection has independent lifecycle and provider-private state. Runtime destruction is guarded until children are gone; provider states are released in reverse successful-initialization order.

Ownership and readiness retain the 0.4.0 invariants: exactly one native close root after acceptance; native readiness differs from TLS readiness; all steps and waits are bounded; release never performs a hidden peer wait.
